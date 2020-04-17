package golibmc

/*
#cgo CFLAGS: -I ./../include
#cgo CXXFLAGS: -I ./../include
#include "c_client.h"
*/
import "C"
import (
	"context"
	"errors"
	"fmt"
	"log"
	"runtime"
	"strconv"
	"strings"
	"sync"
	"time"
	"unsafe"
)

// Configure options
const (
	PollTimeout    = C.CFG_POLL_TIMEOUT
	ConnectTimeout = C.CFG_CONNECT_TIMEOUT
	RetryTimeout   = C.CFG_RETRY_TIMEOUT
	MaxRetries     = C.CFG_MAX_RETRIES
)

// Hash functions
const (
	HashMD5 = iota
	HashFNV1_32
	HashFNV1a32
	HashCRC32
)

// This is the size of the connectionOpener request chan (Client.openerCh).
// This value should be larger than the maximum typical value
// used for client.maxOpen. If maxOpen is significantly larger than
// connectionRequestQueueSize then it is possible for ALL calls into the *Client
// to block until the connectionOpener can satisfy the backlog of requests.
const connectionRequestQueueSize = 1000000

var hashFunctionMapping = map[int]C.hash_function_options_t{
	HashMD5:     C.OPT_HASH_MD5,
	HashFNV1_32: C.OPT_HASH_FNV1_32,
	HashFNV1a32: C.OPT_HASH_FNV1A_32,
	HashCRC32:   C.OPT_HASH_CRC_32,
}

// Credits to:
// https://github.com/bradfitz/gomemcache/blob/master/memcache/memcache.go

// The implementation of connection pool in golibmc is a tailored version of go database/sql.
// https://github.com/golang/go/blob/master/src/database/sql/sql.go

var (
	// ErrCacheMiss means that a Get failed because the item wasn't present.
	ErrCacheMiss = errors.New("libmc: cache miss")

	// ErrCASConflict means that a CompareAndSwap call failed due to the
	// cached value being modified between the Get and the CompareAndSwap.
	// If the cached value was simply evicted rather than replaced,
	// ErrNotStored will be returned instead.
	ErrCASConflict = errors.New("libmc: compare-and-swap conflict")

	// ErrNotStored means that a conditional write operation (i.e. Add or
	// CompareAndSwap) failed because the condition was not satisfied.
	ErrNotStored = errors.New("libmc: item not stored")

	// ErrMalformedKey is returned when an invalid key is used.
	// Keys must be at maximum 250 bytes long, ASCII, and not
	// contain whitespace or control characters.
	ErrMalformedKey = errors.New("malformed: key is too long or contains invalid characters")

	// ErrMemcachedClosed is returned when the memcached connection is
	// already closing.
	ErrMemcachedClosed = errors.New("memcached is closed")

	// ErrBadConn is returned when the connection is out of its maxLifetime, so we decide not to use it.
	ErrBadConn = errors.New("bad conn")

	errTemplate   = "libmc: network error(%s)"
	badConnErrors []error
)

func init() {
	badConnErrors = []error{
		networkError(errorMessage(C.RET_MC_SERVER_ERR)),
		networkError(errorMessage(C.RET_SEND_ERR)),
		networkError(errorMessage(C.RET_CONN_POLL_ERR)),
		networkError(errorMessage(C.RET_RECV_ERR)),
	}
}

func errorMessage(err C.err_code_t) string {
	return C.GoString(C.err_code_to_string(err))
}

func networkError(msg string) error {
	return fmt.Errorf(errTemplate, msg)
}

// DefaultPort memcached port
const DefaultPort = 11211

// Client struct
type Client struct {
	servers        []string
	prefix         string
	noreply        bool
	disableLock    bool
	hashFunc       int
	failover       bool
	connectTimeout C.int
	pollTimeout    C.int
	retryTimeout   C.int
	maxRetries     C.int  // maximum amount of retries. maxRetries <= 0 means unlimited. default is -1.

	lk           sync.Mutex // protects following fields
	freeConns    []*conn
	connRequests map[uint64]chan connRequest // a hashmap of the channels of connRequest
	nextRequest  uint64                      // Next key to use in connRequests.
	numOpen      int                         // number of opened and pending open connections
	// Used to signal the need for new connections
	// a goroutine running connectionOpener() reads on this chan and
	// maybeOpenNewConnections sends on the chan (one send per needed connection)
	// It is closed during client.Quit(). The close tells the connectionOpener
	// goroutine to exit.
	openerCh        chan struct{}
	maxLifetime     time.Duration // maximum amount of time a connection may be reused
	maxOpen         int           // maximum amount of connection num. maxOpen <= 0 means unlimited. default is 1.
	cleanerCh       chan struct{}
	closed          bool
	flushAllEnabled bool
}

// connRequest represents one request for a new connection
// When there are no idle connections available, Client.conn will create
// a new connRequest and put it on the client.connRequests list.
type connRequest struct {
	*conn
	err error
}

// conn wraps a Client with a mutex
type conn struct {
	_imp      unsafe.Pointer
	client    *Client
	createdAt time.Time

	sync.Mutex // guards following
	closed     bool
}

// Item is an item to be got or stored in a memcached server.
// credits to bradfitz/gomemcache
type Item struct {
	// Key is the Item's key (250 bytes maximum).
	Key string

	// Value is the Item's value.
	Value []byte

	// Object is the Item's value for use with a Codec.
	Object interface{}

	// Flags are server-opaque flags whose semantics are entirely
	// up to the app.
	Flags uint32

	// Expiration is the cache expiration time, in seconds: either a relative
	// time from now (up to 1 month), or an absolute Unix epoch time.
	// Zero means the Item has no expiration time.,
	Expiration int64

	// Compare and swap ID.
	casid uint64
}

/*
New to create a memcached client:

servers: is a list of memcached server addresses. Each address can be
in format of hostname[:port] [alias]. port and alias are optional.
If port is not given, default port 11211 will be used. alias will be
used to compute server hash if given, otherwise server hash will be
computed based on host and port (i.e.: If port is not given or it is
equal to 11211, host will be used to compute server hash.
If port is not equal to 11211, host:port will be used).

noreply: whether to enable memcached's noreply behaviour.
default: False

prefix: The key prefix. default: ''

hashFunc: hashing function for keys. possible values:
HashMD5, HashFNV1_32, HashFNV1a32, HashCRC32
NOTE: fnv1_32, fnv1a_32, crc_32 implementations
in libmc are per each spec, but they're not compatible
with corresponding implementions in libmemcached.
NOTE: The hashing algorithm for host mapping on continuum is always md5.

failover: Whether to failover to next server when current server is
not available. default: False

disableLock: Whether to disable a lock of type sync.Mutex which will be
use in every retrieval/storage command. default False.
*/
// FIXME(Harry): the disableLock option is not working now, 'cause we've add a connection pool to golibmc.
func New(servers []string, noreply bool, prefix string, hashFunc int, failover bool, disableLock bool) (client *Client) {
	client = new(Client)
	client.servers = servers
	client.prefix = prefix
	client.noreply = noreply
	client.disableLock = disableLock
	client.hashFunc = hashFunc
	client.failover = failover
	client.flushAllEnabled = false

	client.openerCh = make(chan struct{}, connectionRequestQueueSize)
	client.connRequests = make(map[uint64]chan connRequest)
	// default allow 1 connection per client, it means disable connection pool.
	// users can set this by client.SetConnMaxOpen.
	client.maxOpen = 1

	client.connectTimeout = -1
	client.pollTimeout = -1
	client.retryTimeout = -1
	// users can set this by client.SetMaxRetries.
	client.maxRetries = -1

	go client.connectionOpener()

	return
}

// SimpleNew to create a memcached client with default params
func SimpleNew(servers []string) (client *Client) {
	noreply := false
	prefix := ""
	hashFunc := HashCRC32
	failover := false
	disableLock := false
	return New(
		servers, noreply, prefix, hashFunc,
		failover, disableLock,
	)
}

func finalizer(cn *conn) {
	C.client_destroy(cn._imp)
}

func (client *Client) lock() {
	if !client.disableLock {
		client.lk.Lock()
	}
}

func (client *Client) unlock() {
	if !client.disableLock {
		client.lk.Unlock()
	}
}

// Assumes client.lk is locked.
// If there are connRequests and the connection limit hasn't been reached,
// then tell the connectionOpener to open new connections.
func (client *Client) maybeOpenNewConnections() {
	if client.closed {
		return
	}
	numRequests := len(client.connRequests)
	if client.maxOpen > 0 {
		numCanOpen := client.maxOpen - client.numOpen
		if numRequests > numCanOpen {
			numRequests = numCanOpen
		}
	}
	for numRequests > 0 {
		client.numOpen++ // optimistically
		numRequests--
		client.openerCh <- struct{}{}
	}
}

// Runs in a separate goroutine, opens new connections when requested.
func (client *Client) connectionOpener() {
	for range client.openerCh {
		client.openNewConnection()
	}
}

// Open one new connection
func (client *Client) openNewConnection() {
	// maybeOpenNewConnctions has already executed client.numOpen++ before it sent
	// on client.openerCh. This function must execute client.numOpen-- if the
	// connection fails or is closed before returning.
	if client.closed {
		client.lk.Lock()
		client.numOpen--
		client.lk.Unlock()
		return
	}
	cn, err := client.newConn()
	if err != nil {
		client.lk.Lock()
		defer client.lk.Unlock()
		client.numOpen--
		client.maybeOpenNewConnections()
		return
	}
	client.lk.Lock()
	if !client.putConnLocked(cn, nil) {
		// Don't decrease the client.numOpen here, conn.quit will take care of it.
		client.lk.Unlock()
		cn.quit()
		return
	}
	client.lk.Unlock()
	return
}

// new a conn
func (client *Client) newConn() (*conn, error) {
	cn := conn{
		client:    client,
		_imp:      C.client_create(),
		createdAt: time.Now(),
	}
	runtime.SetFinalizer(&cn, finalizer)

	n := len(client.servers)
	cHosts := make([]*C.char, n)
	cPorts := make([]C.uint32_t, n)
	cAliases := make([]*C.char, n)

	for i, srv := range client.servers {
		addrAndAlias := strings.Split(srv, " ")

		addr := addrAndAlias[0]
		if len(addrAndAlias) == 2 {
			cAlias := C.CString(addrAndAlias[1])
			defer C.free(unsafe.Pointer(cAlias))
			cAliases[i] = cAlias
		}

		hostAndPort := strings.Split(addr, ":")
		host := hostAndPort[0]
		cHost := C.CString(host)
		defer C.free(unsafe.Pointer(cHost))
		cHosts[i] = cHost

		if len(hostAndPort) == 2 {
			port, err := strconv.Atoi(hostAndPort[1])
			if err != nil {
				return nil, err
			}
			cPorts[i] = C.uint32_t(port)
		} else {
			cPorts[i] = C.uint32_t(DefaultPort)
		}
	}

	failoverInt := 0
	if client.failover {
		failoverInt = 1
	}

	C.client_init(
		cn._imp,
		(**C.char)(unsafe.Pointer(&cHosts[0])),
		(*C.uint32_t)(unsafe.Pointer(&cPorts[0])),
		C.size_t(n),
		(**C.char)(unsafe.Pointer(&cAliases[0])),
		C.int(failoverInt),
	)

	cn.configHashFunction(int(hashFunctionMapping[client.hashFunc]))
	if client.retryTimeout >= 0 {
		C.client_config(cn._imp, RetryTimeout, client.retryTimeout)
	}
	if client.pollTimeout >= 0 {
		C.client_config(cn._imp, PollTimeout, client.pollTimeout)
	}
	if client.connectTimeout >= 0 {
		C.client_config(cn._imp, ConnectTimeout, client.connectTimeout)
	}
	if client.maxRetries >= 0 {
		C.client_config(cn._imp, MaxRetries, client.maxRetries)
	}
	return &cn, nil
}

// putConn adds a connection to the client's free pool or satisfy a connRequest,
// if the conn is not bad.
// err is optionally the last error that occurred on this connection.
func (client *Client) putConn(cn *conn, err error) error {
	client.lk.Lock()
	if isBadConnErr(err) || !client.putConnLocked(cn, nil) {
		client.lk.Unlock()
		err1 := cn.quit()
		if err1 != nil {
			log.Printf("Failed cn.quit: %v", err1)
		}
		return err
	}
	client.lk.Unlock()
	return err
}

// Satisfy a connRequest or put the conn in the idle pool and return true
// or return false.
// putConnLocked will satisfy a connRequest if there is one, or it will
// return the *conn to the freeConn list (currently, no idle connection limit).
// If err != nil, the value of cn is ignored.
// If err == nil, then cn must not equal nil.
// If a connRequest was fulfilled or the *conn was placed in the
// freeConn list, then true is returned, otherwise false is returned.
func (client *Client) putConnLocked(cn *conn, err error) bool {
	if client.closed {
		return false
	}
	if client.maxOpen > 0 && client.maxOpen < client.numOpen {
		return false
	}
	if len(client.connRequests) > 0 {
		// Get a request from queue and satisfy it.
		var req chan connRequest
		var reqKey uint64
		for reqKey, req = range client.connRequests {
			break
		}
		// Remove from pending requests.
		delete(client.connRequests, reqKey)
		req <- connRequest{
			conn: cn,
			err:  err,
		}
	} else {
		client.freeConns = append(client.freeConns, cn)
		client.startCleanerLocked()
	}
	return true
}

// Client.conn returns a single connection by either opening a new connection
// or returning an existing connection from the connection pool. Client.conn will
// block until either a connection is returned or ctx is canceled.
//
// Every conn should be returned to the connection pool after use by
// calling Client.putConn.
func (client *Client) conn(ctx context.Context) (*conn, error) {
	cn, err := client._conn(ctx, true)
	if err == nil {
		return cn, nil
	}
	// If we get ErrBadConn, it means the free conn is reach its maxLifetime.
	// So we decide to force create a new connection.
	if err == ErrBadConn {
		return client._conn(ctx, false)
	}
	return cn, err
}

// _conn returns a newly-opened or cached *conn.
func (client *Client) _conn(ctx context.Context, useFreeConn bool) (*conn, error) {
	client.lk.Lock()
	if client.closed {
		client.lk.Unlock()
		return nil, ErrMemcachedClosed
	}
	// Check if the context is expired.
	select {
	default:
	case <-ctx.Done():
		client.lk.Unlock()
		return nil, ctx.Err()
	}
	lifetime := client.maxLifetime

	// Prefer a free connection, if possible.
	numFree := len(client.freeConns)
	if useFreeConn && numFree > 0 {
		cn := client.freeConns[0]
		copy(client.freeConns, client.freeConns[1:])
		client.freeConns = client.freeConns[:numFree-1]
		client.lk.Unlock()
		if cn.expired(lifetime) {
			cn.quit()
			return nil, ErrBadConn
		}
		return cn, nil
	}

	// Out of free connections or we were asked not to use one. If we're not
	// allowed to open any more connections, make a request and wait.
	if client.maxOpen > 0 && client.maxOpen <= client.numOpen {
		// New connection request
		// Make the connRequest channel. It's buffered so that the
		// connectionOpener doesn't block while waiting for the req to be read.
		req := make(chan connRequest, 1)

		// It is assumed that nextRequest will not overflow.
		reqKey := client.nextRequest
		client.nextRequest++
		client.connRequests[reqKey] = req
		client.lk.Unlock()

		// Timeout the connection request with the context.
		select {
		case <-ctx.Done():
			// Remove the connection request and ensure no value has been sent
			// on it after removing.
			client.lk.Lock()
			delete(client.connRequests, reqKey)
			client.lk.Unlock()

			// Make sure we put the conn back if we got one.
			select {
			case ret, ok := <-req:
				if ok {
					client.putConn(ret.conn, ret.err)
				}
			default:
			}
			return nil, ctx.Err()
		case ret, ok := <-req:
			if !ok {
				return nil, ErrMemcachedClosed
			}
			return ret.conn, ret.err
		}
	}

	// Make a new connection
	client.numOpen++ // optimistically
	client.lk.Unlock()
	newCn, err := client.newConn()
	if err != nil {
		client.lk.Lock()
		defer client.lk.Unlock()
		client.numOpen-- // correct for earlier optimism
		client.maybeOpenNewConnections()
		return nil, err
	}
	return newCn, nil
}

// SetConnMaxLifetime sets the maximum amount of time a connection may be reused.
//
// Expired connections may be closed lazily before reuse.
//
// If d <= 0, connections are reused forever.
func (client *Client) SetConnMaxLifetime(d time.Duration) {
	if d < 0 {
		d = 0
	}
	client.lk.Lock()
	// wake cleaner up when maxLifetime is shortened.
	if d > 0 && d < client.maxLifetime && client.cleanerCh != nil {
		select {
		case client.cleanerCh <- struct{}{}:
		default:
		}
	}
	client.maxLifetime = d
	client.startCleanerLocked()
	client.lk.Unlock()
}

// SetConnMaxOpen sets the maximum amount of opening connections.
func (client *Client) SetConnMaxOpen(maxOpen int) {
	client.lk.Lock()
	defer client.lk.Unlock()
	client.maxOpen = maxOpen
}

// SetMaxRetries sets the maximum amount of retries.
func (client *Client) SetMaxRetries(maxRetries int) {
	client.lk.Lock()
	defer client.lk.Unlock()
	client.maxRetries = C.int(maxRetries)
}

func (client *Client) needStartCleaner() bool {
	return client.maxLifetime > 0 &&
		client.numOpen > 0 &&
		client.cleanerCh == nil
}

// startCleanerLocked starts connectionCleaner if needed.
func (client *Client) startCleanerLocked() {
	if client.needStartCleaner() {
		client.cleanerCh = make(chan struct{}, 1)
		go client.connectionCleaner(client.maxLifetime)
	}
}

func (client *Client) connectionCleaner(d time.Duration) {
	const minInterval = time.Second

	if d < minInterval {
		d = minInterval
	}
	t := time.NewTimer(d)

	for {
		select {
		case <-t.C:
		case <-client.cleanerCh: // maxLifetime was changed or memcached was closed.
		}

		client.lk.Lock()
		d = client.maxLifetime
		if client.closed || client.numOpen == 0 || d <= 0 {
			client.cleanerCh = nil
			client.lk.Unlock()
			return
		}

		expiredSince := time.Now().Add(-d)
		var closing []*conn
		for i := 0; i < len(client.freeConns); i++ {
			cn := client.freeConns[i]
			if cn.createdAt.Before(expiredSince) {
				closing = append(closing, cn)
				last := len(client.freeConns) - 1
				client.freeConns[i] = client.freeConns[last]
				client.freeConns[last] = nil
				client.freeConns = client.freeConns[:last]
				i--
			}
		}
		client.lk.Unlock()

		for _, cn := range closing {
			if err := cn.quit(); err != nil {
				log.Println("Failed conn.quit", err)
			}
		}

		if d < minInterval {
			d = minInterval
		}
		t.Reset(d)
	}
}

func (cn *conn) configHashFunction(val int) {
	cn.Lock()
	defer cn.Unlock()

	C.client_config(cn._imp, C.CFG_HASH_FUNCTION, C.int(val))
}

// ConfigTimeout Keys:
//	PollTimeout
//	ConnectTimeout
//	RetryTimeout
//
// timeout should of type time.Duration
func (client *Client) ConfigTimeout(cCfgKey C.config_options_t, timeout time.Duration) {
	client.lk.Lock()
	defer client.lk.Unlock()
	switch cCfgKey {
	case RetryTimeout:
		client.retryTimeout = C.int(timeout / time.Second)
	case PollTimeout:
		client.pollTimeout = C.int(timeout / time.Millisecond)
	case ConnectTimeout:
		client.connectTimeout = C.int(timeout / time.Millisecond)
	}
}

// GetServerAddressByKey will return the address of the memcached
// server where a key is stored (assume all memcached servers are
// accessiable and wonot establish any connections. )
func (client *Client) GetServerAddressByKey(key string) string {
	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))

	cn, err := client.conn(context.Background())
	if err != nil {
		return ""
	}
	defer func() {
		client.putConn(cn, err)
	}()
	cServerAddr := C.client_get_server_address_by_key(cn._imp, cKey, cKeyLen)
	return C.GoString(cServerAddr)
}

// GetRealtimeServerAddressByKey will return the address of the memcached
// server where a key is stored. (Will try to connect to
// corresponding memcached server and may failover accordingly. )
// if no server is avaiable, an empty string will be returned.
func (client *Client) GetRealtimeServerAddressByKey(key string) string {
	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cn, err := client.conn(context.Background())
	if err != nil {
		return ""
	}
	defer func() {
		client.putConn(cn, err)
	}()
	cServerAddr := C.client_get_realtime_server_address_by_key(cn._imp, cKey, cKeyLen)
	if cServerAddr != nil {
		return C.GoString(cServerAddr)
	}
	return ""
}

func (client *Client) removePrefix(key string) string {
	if len(client.prefix) == 0 {
		return key
	}
	if strings.HasPrefix(key, "?") {
		return strings.Join([]string{"?", strings.Replace(key[1:], client.prefix, "", 1)}, "")
	}
	return strings.Replace(key, client.prefix, "", 1)
}

func (client *Client) addPrefix(key string) string {
	if len(client.prefix) == 0 {
		return key
	}
	if strings.HasPrefix(key, "?") {
		return strings.Join([]string{"?", client.prefix, key[1:]}, "")
	}
	return strings.Join([]string{client.prefix, key}, "")
}

func (client *Client) store(cmd string, item *Item) error {
	key := client.addPrefix(item.Key)

	cKey := C.CString(key)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(key))
	cFlags := C.flags_t(item.Flags)
	cExptime := C.exptime_t(item.Expiration)
	cNoreply := C.bool(client.noreply)
	cValue := C.CString(string(item.Value))
	defer C.free(unsafe.Pointer(cValue))
	cValueSize := C.size_t(len(item.Value))

	var rst **C.message_result_t
	var n C.size_t

	var errCode C.err_code_t

	cn, err := client.conn(context.Background())
	if err != nil {
		return err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	switch cmd {
	case "set":
		errCode = C.client_set(
			cn._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "add":
		errCode = C.client_add(
			cn._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "replace":
		errCode = C.client_replace(
			cn._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "prepend":
		errCode = C.client_prepend(
			cn._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "append":
		errCode = C.client_append(
			cn._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "cas":
		cCasUnique := C.cas_unique_t(item.casid)
		errCode = C.client_cas(
			cn._imp, &cKey, &cKeyLen, &cFlags, cExptime, &cCasUnique,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	}
	defer C.client_destroy_message_result(cn._imp)

	if errCode == C.RET_OK {
		if client.noreply {
			return nil
		} else if int(n) == 1 {
			switch (*rst).type_ {
			case C.MSG_STORED:
				return nil
			case C.MSG_NOT_STORED:
				return ErrNotStored
			case C.MSG_EXISTS:
				return ErrCASConflict
			case C.MSG_NOT_FOUND:
				return ErrCacheMiss
			}
		}
	} else if errCode == C.RET_INVALID_KEY_ERR {
		return ErrMalformedKey
	}

	return networkError(errorMessage(errCode))
}

// Add is a storage command, return without error only when the key is empty
func (client *Client) Add(item *Item) error {
	return client.store("add", item)
}

// Replace is a storage command, return without error only when the key is not empty
func (client *Client) Replace(item *Item) error {
	return client.store("replace", item)
}

// Prepend value to an existed key
func (client *Client) Prepend(item *Item) error {
	return client.store("prepend", item)
}

// Append value to an existed key
func (client *Client) Append(item *Item) error {
	return client.store("append", item)
}

// Set value to a key
func (client *Client) Set(item *Item) error {
	return client.store("set", item)
}

// SetMulti will set multi values at once
func (client *Client) SetMulti(items []*Item) (failedKeys []string, err error) {
	nItems := len(items)
	cKeys := make([]*C.char, nItems)
	cKeyLens := make([]C.size_t, nItems)
	cValues := make([]*C.char, nItems)
	cValueSizes := make([]C.size_t, nItems)
	cFlagsList := make([]C.flags_t, nItems)

	for i, item := range items {
		rawKey := client.addPrefix(item.Key)

		cKey := C.CString(rawKey)
		defer C.free(unsafe.Pointer(cKey))
		cKeys[i] = cKey

		cKeyLen := C.size_t(len(rawKey))
		cKeyLens[i] = cKeyLen

		cVal := C.CString(string(item.Value))
		defer C.free(unsafe.Pointer(cVal))
		cValues[i] = cVal

		cValueSize := C.size_t(len(item.Value))
		cValueSizes[i] = cValueSize

		cFlags := C.flags_t(item.Flags)
		cFlagsList[i] = cFlags
	}

	cExptime := C.exptime_t(items[0].Expiration)
	cNoreply := C.bool(client.noreply)
	cNItems := C.size_t(nItems)

	var results **C.message_result_t
	var n C.size_t

	cn, err := client.conn(context.Background())
	if err != nil {
		return []string{}, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_set(
		cn._imp,
		(**C.char)(&cKeys[0]),
		(*C.size_t)(&cKeyLens[0]),
		(*C.flags_t)(&cFlagsList[0]),
		cExptime,
		nil,
		cNoreply,
		(**C.char)(&cValues[0]),
		(*C.size_t)(&cValueSizes[0]),
		cNItems,
		&results, &n,
	)
	defer C.client_destroy_message_result(cn._imp)
	if errCode == C.RET_OK {
		return []string{}, nil
	}

	if errCode == C.RET_INVALID_KEY_ERR {
		err = ErrMalformedKey
	} else {
		err = networkError(errorMessage(errCode))
	}

	sr := unsafe.Sizeof(*results)
	storedKeySet := make(map[string]struct{})
	for i := 0; i < int(n); i++ {
		if (*results).type_ == C.MSG_STORED {
			storedKey := C.GoStringN((*results).key, C.int((*results).key_len))
			storedKeySet[storedKey] = struct{}{}
		}

		results = (**C.message_result_t)(
			unsafe.Pointer(uintptr(unsafe.Pointer(results)) + sr),
		)
	}
	failedKeys = make([]string, len(items)-len(storedKeySet))
	i := 0
	for _, item := range items {
		if _, contains := storedKeySet[item.Key]; contains {
			continue
		}
		failedKeys[i] = client.removePrefix(item.Key)
		i++
	}
	return failedKeys, err
}

// Cas is short for Compare And Swap
func (client *Client) Cas(item *Item) error {
	return client.store("cas", item)
}

// Delete a key
func (client *Client) Delete(key string) error {
	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cNoreply := C.bool(client.noreply)

	var rst **C.message_result_t
	var n C.size_t

	cn, err := client.conn(context.Background())
	if err != nil {
		return err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_delete(
		cn._imp, &cKey, &cKeyLen, cNoreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(cn._imp)

	if errCode == C.RET_OK {
		if client.noreply {
			return nil
		} else if int(n) == 1 {
			if (*rst).type_ == C.MSG_DELETED {
				return nil
			} else if (*rst).type_ == C.MSG_NOT_FOUND {
				return ErrCacheMiss
			}
		}
	} else if errCode == C.RET_INVALID_KEY_ERR {
		return ErrMalformedKey
	}

	return networkError(errorMessage(errCode))
}

// DeleteMulti will delete multi keys at once
func (client *Client) DeleteMulti(keys []string) (failedKeys []string, err error) {
	var rawKeys []string
	if len(client.prefix) == 0 {
		rawKeys = keys
	} else {
		rawKeys = make([]string, len(keys))
		for i, key := range keys {
			rawKeys[i] = client.addPrefix(key)
		}
	}

	nKeys := len(rawKeys)
	cNKeys := C.size_t(nKeys)
	cKeys := make([]*C.char, nKeys)
	cKeyLens := make([]C.size_t, nKeys)
	cNoreply := C.bool(client.noreply)

	var results **C.message_result_t
	var n C.size_t

	for i, key := range rawKeys {
		cKey := C.CString(key)
		defer C.free(unsafe.Pointer(cKey))
		cKeys[i] = cKey

		cKeyLen := C.size_t(len(key))
		cKeyLens[i] = cKeyLen
	}
	cn, err := client.conn(context.Background())
	if err != nil {
		return []string{}, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_delete(
		cn._imp, (**C.char)(&cKeys[0]), (*C.size_t)(&cKeyLens[0]), cNoreply, cNKeys,
		&results,
		&n,
	)
	defer C.client_destroy_message_result(cn._imp)

	switch errCode {
	case C.RET_OK:
		err = nil
		return
	case C.RET_INVALID_KEY_ERR:
		err = ErrMalformedKey
	default:
		err = networkError(errorMessage(errCode))
	}

	if client.noreply {
		failedKeys = keys
		return
	}

	deletedKeySet := make(map[string]struct{})
	sr := unsafe.Sizeof(*results)
	for i := 0; i < int(n); i++ {
		if (*results).type_ == C.MSG_DELETED {
			deletedKey := C.GoStringN((*results).key, C.int((*results).key_len))
			deletedKeySet[deletedKey] = struct{}{}
		}

		results = (**C.message_result_t)(
			unsafe.Pointer(uintptr(unsafe.Pointer(results)) + sr),
		)
	}
	err = networkError(errorMessage(errCode))
	failedKeys = make([]string, len(rawKeys)-len(deletedKeySet))

	i := 0
	for _, key := range rawKeys {
		if _, contains := deletedKeySet[key]; contains {
			continue
		}
		failedKeys[i] = client.removePrefix(key)
		i++
	}
	return
}

func (client *Client) getOrGets(cmd string, key string) (item *Item, err error) {
	cn, err := client.conn(context.Background())
	if err != nil {
		return nil, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	var rst **C.retrieval_result_t
	var n C.size_t

	var errCode C.err_code_t
	switch cmd {
	case "get":
		errCode = C.client_get(cn._imp, &cKey, &cKeyLen, 1, &rst, &n)
	case "gets":
		errCode = C.client_gets(cn._imp, &cKey, &cKeyLen, 1, &rst, &n)
	}

	defer C.client_destroy_retrieval_result(cn._imp)

	if errCode != C.RET_OK {
		if errCode == C.RET_INVALID_KEY_ERR {
			err = ErrMalformedKey
		} else {
			err = networkError(errorMessage(errCode))
		}
		return
	}

	if int(n) == 0 {
		err = ErrCacheMiss
		return
	}

	dataBlock := C.GoBytes(unsafe.Pointer((*rst).data_block), C.int((*rst).bytes))
	flags := uint32((*rst).flags)
	if cmd == "get" {
		item = &Item{Key: key, Value: dataBlock, Flags: flags}
		return
	}

	casid := uint64((*rst).cas_unique)
	item = &Item{Key: key, Value: dataBlock, Flags: flags, casid: casid}
	return
}

// Get is a retrieval command. It will return Item or nil
func (client *Client) Get(key string) (*Item, error) {
	return client.getOrGets("get", key)
}

// Gets is a retrieval command. It will return Item(with casid) or nil
func (client *Client) Gets(key string) (*Item, error) {
	return client.getOrGets("gets", key)
}

// GetMulti will return a map of multi values
func (client *Client) GetMulti(keys []string) (rv map[string]*Item, err error) {
	nKeys := len(keys)
	var rawKeys []string
	if len(client.prefix) == 0 {
		rawKeys = keys
	} else {
		rawKeys = make([]string, len(keys))
		for i, key := range keys {
			rawKeys[i] = client.addPrefix(key)
		}
	}

	cKeys := make([]*C.char, nKeys)
	cKeyLens := make([]C.size_t, nKeys)
	cNKeys := C.size_t(nKeys)

	for i, rawKey := range rawKeys {
		cKey := C.CString(rawKey)
		defer C.free(unsafe.Pointer(cKey))
		cKeyLen := C.size_t(len(rawKey))
		cKeys[i] = cKey
		cKeyLens[i] = cKeyLen
	}

	var rst **C.retrieval_result_t
	var n C.size_t

	cn, err1 := client.conn(context.Background())
	if err1 != nil {
		err = err1
		return
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_get(cn._imp, &cKeys[0], &cKeyLens[0], cNKeys, &rst, &n)
	defer C.client_destroy_retrieval_result(cn._imp)

	switch errCode {
	case C.RET_OK:
		err = nil
	case C.RET_INVALID_KEY_ERR:
		err = ErrMalformedKey
	default:
		err = networkError(errorMessage(errCode))
	}

	if err == nil && len(keys) != int(n) {
		err = ErrCacheMiss
	}

	if int(n) == 0 {
		return
	}

	sr := unsafe.Sizeof(*rst)
	rv = make(map[string]*Item, int(n))
	for i := 0; i < int(n); i++ {
		rawKey := C.GoStringN((*rst).key, C.int((*rst).key_len))
		dataBlock := C.GoBytes(unsafe.Pointer((*rst).data_block), C.int((*rst).bytes))
		flags := uint32((*rst).flags)
		key := client.removePrefix(rawKey)
		rv[key] = &Item{Key: key, Value: dataBlock, Flags: flags}
		rst = (**C.retrieval_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	return
}

// Touch command
func (client *Client) Touch(key string, expiration int64) error {
	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cExptime := C.exptime_t(expiration)
	cNoreply := C.bool(client.noreply)

	var rst **C.message_result_t
	var n C.size_t

	cn, err := client.conn(context.Background())
	if err != nil {
		return err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_touch(
		cn._imp, &cKey, &cKeyLen, cExptime, cNoreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(cn._imp)

	switch errCode {
	case C.RET_OK:
		if client.noreply {
			return nil
		} else if int(n) == 1 {
			if (*rst).type_ == C.MSG_TOUCHED {
				return nil
			} else if (*rst).type_ == C.MSG_NOT_FOUND {
				return ErrCacheMiss
			}
		}
	case C.RET_INVALID_KEY_ERR:
		return ErrMalformedKey
	}
	return networkError(errorMessage(errCode))
}

func (client *Client) incrOrDecr(cmd string, key string, delta uint64) (uint64, error) {
	rawKey := client.addPrefix(key)
	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cDelta := C.uint64_t(delta)
	cNoreply := C.bool(client.noreply)

	var rst *C.unsigned_result_t
	var n C.size_t

	var errCode C.err_code_t

	cn, err := client.conn(context.Background())
	if err != nil {
		return 0, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	switch cmd {
	case "incr":
		errCode = C.client_incr(
			cn._imp, cKey, cKeyLen, cDelta, cNoreply,
			&rst, &n,
		)
	case "decr":
		errCode = C.client_decr(
			cn._imp, cKey, cKeyLen, cDelta, cNoreply,
			&rst, &n,
		)

	}

	defer C.client_destroy_unsigned_result(cn._imp)

	if errCode == C.RET_OK {
		if client.noreply {
			return 0, nil
		} else if int(n) == 1 {
			if rst == nil {
				return 0, ErrCacheMiss
			}
			return uint64(rst.value), nil
		}
	} else if errCode == C.RET_INVALID_KEY_ERR {
		return 0, ErrMalformedKey
	}

	return 0, networkError(errorMessage(errCode))
}

// Incr will increase the value in key by delta
func (client *Client) Incr(key string, delta uint64) (uint64, error) {
	return client.incrOrDecr("incr", key, delta)
}

// Decr will decrease the value in key by delta
func (client *Client) Decr(key string, delta uint64) (uint64, error) {
	return client.incrOrDecr("decr", key, delta)
}

// Version will return a map reflecting versions of each memcached server
func (client *Client) Version() (map[string]string, error) {
	var rst *C.broadcast_result_t
	var n C.size_t
	rv := make(map[string]string)

	cn, err := client.conn(context.Background())
	if err != nil {
		return rv, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_version(cn._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(cn._imp)
	sr := unsafe.Sizeof(*rst)

	for i := 0; i < int(n); i++ {
		if rst.lines == nil || rst.line_lens == nil {
			continue
		}

		host := C.GoString(rst.host)
		version := C.GoStringN(*rst.lines, C.int(*rst.line_lens))
		rv[host] = version
		rst = (*C.broadcast_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	if errCode != C.RET_OK {
		return rv, networkError(errorMessage(errCode))
	}

	return rv, nil
}

// Stats will return a map reflecting stats map of each memcached server
func (client *Client) Stats() (map[string](map[string]string), error) {
	var rst *C.broadcast_result_t
	var n C.size_t

	rv := make(map[string](map[string]string))

	cn, err := client.conn(context.Background())
	if err != nil {
		return rv, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_stats(cn._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(cn._imp)

	sr := unsafe.Sizeof(*rst)

	for i := 0; i < int(n); i++ {
		if rst.lines == nil || rst.line_lens == nil {
			continue
		}
		host := C.GoString(rst.host)
		nMetrics := int(rst.len)

		rv[host] = make(map[string]string)

		cLines := rst.lines
		cLineLens := rst.line_lens

		sLine := unsafe.Sizeof(*cLines)
		sLineLen := unsafe.Sizeof(*cLineLens)
		for j := 0; j < nMetrics; j++ {
			metricLine := C.GoStringN(*cLines, C.int(*cLineLens))
			kv := strings.SplitN(metricLine, " ", 2)
			rv[host][kv[0]] = kv[1]
			cLines = (**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(cLines)) + sLine))
			cLineLens = (*C.size_t)(unsafe.Pointer(uintptr(unsafe.Pointer(cLineLens)) + sLineLen))
		}

		rst = (*C.broadcast_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	if errCode != C.RET_OK {
		return rv, networkError(errorMessage(errCode))
	}

	return rv, nil
}

// Enable/Disable the flush_all feature
func (client *Client) ToggleFlushAllFeature(enabled bool) {
	client.flushAllEnabled = enabled
}

// FlushAll will flush all memcached servers
// You must call  ToggleFlushAllFeature(True) first to
// enable this feature.
func (client *Client) FlushAll() ([]string, error) {
	var rst *C.broadcast_result_t
	var n C.size_t
	flushedHosts := []string{}

	cn, err := client.conn(context.Background())
	C.client_toggle_flush_all_feature(
		cn._imp, C.bool(client.flushAllEnabled),
	)
	if err != nil {
		return flushedHosts, err
	}
	defer func() {
		client.putConn(cn, err)
	}()

	errCode := C.client_flush_all(cn._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(cn._imp)
	sr := unsafe.Sizeof(*rst)

	for i := 0; i < int(n); i++ {
		host := C.GoString(rst.host)
		if rst.msg_type == C.MSG_OK {
			flushedHosts = append(flushedHosts, host)
		}

		rst = (*C.broadcast_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	if errCode != C.RET_OK {
		if errCode == C.RET_PROGRAMMING_ERR {
			return flushedHosts, errors.New(
				"client.ToggleFlushAllFeature(true) to enable flush_all",
			)
		}
		return flushedHosts, networkError(errorMessage(errCode))
	}

	return flushedHosts, nil
}

// Quit will close the sockets to each memcached server
func (client *Client) Quit() error {
	client.lk.Lock()
	if client.closed {
		client.lk.Unlock()
		return nil
	}

	close(client.openerCh)
	if client.cleanerCh != nil {
		close(client.cleanerCh)
	}
	for _, cr := range client.connRequests {
		close(cr)
	}
	client.closed = true
	client.lk.Unlock()
	var err error
	for _, cn := range client.freeConns {
		err1 := cn.quit()
		if err1 != nil {
			err = err1
		}
	}
	client.lk.Lock()
	client.freeConns = nil
	client.lk.Unlock()

	// FIXME(Harry): We use empty context everywhere, and it is useless.
	//	We should cancel ctx here to make it useful.

	return err
}

func (cn *conn) expired(timeout time.Duration) bool {
	if timeout <= 0 {
		return false
	}
	return cn.createdAt.Add(timeout).Before(time.Now())
}

// FIXME(Harry): We are using quit everywhere when the conn is failed,
//	I think we can just close socket instead of send quit, it should save some time.
//	So don't mix up Quit and Close, implement a Close function plz.
func (cn *conn) quit() error {
	cn.Lock()
	defer cn.Unlock()
	if cn.closed {
		return errors.New("duplicate conn close")
	}
	cn.closed = true
	errCode := C.client_quit(cn._imp)
	C.client_destroy_broadcast_result(cn._imp)
	cn.client.lk.Lock()
	defer cn.client.lk.Unlock()
	cn.client.numOpen--
	// FIXME(Harry): I think we can use isBadConnErr here.
	if errCode == C.RET_OK || errCode == C.RET_CONN_POLL_ERR {
		cn.client.maybeOpenNewConnections()
		return nil
	}
	return networkError(errorMessage(errCode))
}

func isBadConnErr(err error) (r bool) {
	if err == nil {
		return
	}
	if err == ErrBadConn {
		r = true
		return
	}
	for _, be := range badConnErrors {
		if err.Error() == be.Error() {
			r = true
			break
		}
	}
	return
}
