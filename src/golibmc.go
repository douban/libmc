package golibmc

/*
#cgo CFLAGS: -I ./../include
#cgo CXXFLAGS: -I ./../include
#include "c_client.h"
*/
import "C"
import (
	"errors"
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
)

// Hash functions
const (
	HashMD5 = iota
	HashFNV1_32
	HashFNV1a32
	HashCRC32
)

var hashFunctionMapping = map[int]C.hash_function_options_t{
	HashMD5:     C.OPT_HASH_MD5,
	HashFNV1_32: C.OPT_HASH_FNV1_32,
	HashFNV1a32: C.OPT_HASH_FNV1A_32,
	HashCRC32:   C.OPT_HASH_CRC_32,
}

var errorMessage = map[C.int]string{
	C.RET_SEND_ERR:         "send_error",
	C.RET_RECV_ERR:         "recv_error",
	C.RET_CONN_POLL_ERR:    "conn_poll_error",
	C.RET_POLL_TIMEOUT_ERR: "poll_timeout_error",
	C.RET_POLL_ERR:         "poll_error",
	C.RET_MC_SERVER_ERR:    "server_error",

	C.RET_PROGRAMMING_ERR:       "programming_error",
	C.RET_INVALID_KEY_ERR:       "invalid_key_error",
	C.RET_INCOMPLETE_BUFFER_ERR: "incomplete_buffer_error",
	C.RET_OK:                    "ok",
}

// Credits to:
// https://github.com/bradfitz/gomemcache/blob/master/memcache/memcache.go

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
)

func networkError(msg string) error {
	return errors.New("libmc: network error(" + msg + ")")
}

// Default memcached port
const DefaultPort = 11211

// Client struct
type Client struct {
	_imp        unsafe.Pointer
	servers     []string
	prefix      string
	noreply     bool
	disableLock bool
	lk          sync.Mutex
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
func New(servers []string, noreply bool, prefix string, hashFunc int, failover bool, disableLock bool) (client *Client) {
	client = new(Client)
	client._imp = C.client_create()
	runtime.SetFinalizer(client, finalizer)

	n := len(servers)
	cHosts := make([]*C.char, n)
	cPorts := make([]C.uint32_t, n)
	cAliases := make([]*C.char, n)

	for i, srv := range servers {
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
				return nil
			}
			cPorts[i] = C.uint32_t(port)
		} else {
			cPorts[i] = C.uint32_t(DefaultPort)
		}
	}

	failoverInt := 0
	if failover {
		failoverInt = 1
	}

	C.client_init(
		client._imp,
		(**C.char)(unsafe.Pointer(&cHosts[0])),
		(*C.uint32_t)(unsafe.Pointer(&cPorts[0])),
		C.size_t(n),
		(**C.char)(unsafe.Pointer(&cAliases[0])),
		C.int(failoverInt),
	)

	client.configHashFunction(int(hashFunctionMapping[hashFunc]))
	client.servers = servers
	client.prefix = prefix
	client.noreply = noreply
	client.disableLock = disableLock
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

func finalizer(client *Client) {
	C.client_destroy(client._imp)
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

func (client *Client) configHashFunction(val int) {
	client.lock()
	defer client.unlock()

	C.client_config(client._imp, C.CFG_HASH_FUNCTION, C.int(val))
}

// ConfigTimeout Keys:
//	PollTimeout
//	ConnectTimeout
//	RetryTimeout
//
// timeout should of type time.Duration
func (client *Client) ConfigTimeout(cCfgKey C.config_options_t, timeout time.Duration) {
	client.lock()
	defer client.unlock()
	var cTimeout C.int
	if cCfgKey == C.CFG_RETRY_TIMEOUT {
		cTimeout = C.int(timeout / time.Second)
	} else {
		cTimeout = C.int(timeout / time.Microsecond)
	}
	C.client_config(client._imp, cCfgKey, cTimeout)
}

// GetServerAddressByKey will return the address of the memcached
// server where a key is stored
func (client *Client) GetServerAddressByKey(key string) string {
	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cServerAddr := C.client_get_server_address_by_key(client._imp, cKey, cKeyLen)
	return C.GoString(cServerAddr)
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
	client.lock()
	defer client.unlock()

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

	var errCode C.int

	switch cmd {
	case "set":
		errCode = C.client_set(
			client._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "add":
		errCode = C.client_add(
			client._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "replace":
		errCode = C.client_replace(
			client._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "prepend":
		errCode = C.client_prepend(
			client._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "append":
		errCode = C.client_append(
			client._imp, &cKey, &cKeyLen, &cFlags, cExptime, nil,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	case "cas":
		cCasUnique := C.cas_unique_t(item.casid)
		errCode = C.client_cas(
			client._imp, &cKey, &cKeyLen, &cFlags, cExptime, &cCasUnique,
			cNoreply, &cValue, &cValueSize, 1, &rst, &n,
		)
	}
	defer C.client_destroy_message_result(client._imp)

	if errCode == 0 {
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

	return networkError(errorMessage[errCode])
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
	client.lock()
	defer client.unlock()

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

	errCode := C.client_set(
		client._imp,
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
	defer C.client_destroy_message_result(client._imp)
	if errCode == 0 {
		return []string{}, nil
	}

	if errCode == C.RET_INVALID_KEY_ERR {
		err = ErrMalformedKey
	} else {
		err = networkError(errorMessage[errCode])
	}

	sr := unsafe.Sizeof(*results)
	storedKeySet := make(map[string]struct{})
	for i := 0; i <= int(n); i++ {
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
	client.lock()
	defer client.unlock()

	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cNoreply := C.bool(client.noreply)

	var rst **C.message_result_t
	var n C.size_t

	errCode := C.client_delete(
		client._imp, &cKey, &cKeyLen, cNoreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(client._imp)

	if errCode == 0 {
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

	return networkError(errorMessage[errCode])
}

// DeleteMulti will delete multi keys at once
func (client *Client) DeleteMulti(keys []string) (failedKeys []string, err error) {
	client.lock()
	defer client.unlock()

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
	errCode := C.client_delete(
		client._imp, (**C.char)(&cKeys[0]), (*C.size_t)(&cKeyLens[0]), cNoreply, cNKeys,
		&results,
		&n,
	)
	defer C.client_destroy_message_result(client._imp)

	switch errCode {
	case 0:
		err = nil
		return
	case C.RET_INVALID_KEY_ERR:
		err = ErrMalformedKey
	default:
		err = networkError(errorMessage[errCode])
	}

	if client.noreply {
		failedKeys = keys
		return
	}

	deletedKeySet := make(map[string]struct{})
	sr := unsafe.Sizeof(*results)
	for i := 0; i <= int(n); i++ {
		if (*results).type_ == C.MSG_DELETED {
			deletedKey := C.GoStringN((*results).key, C.int((*results).key_len))
			deletedKeySet[deletedKey] = struct{}{}
		}

		results = (**C.message_result_t)(
			unsafe.Pointer(uintptr(unsafe.Pointer(results)) + sr),
		)
	}
	err = networkError(errorMessage[errCode])
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
	client.lock()
	defer client.unlock()

	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	var rst **C.retrieval_result_t
	var n C.size_t

	var errCode C.int
	switch cmd {
	case "get":
		errCode = C.client_get(client._imp, &cKey, &cKeyLen, 1, &rst, &n)
	case "gets":
		errCode = C.client_gets(client._imp, &cKey, &cKeyLen, 1, &rst, &n)
	}

	defer C.client_destroy_retrieval_result(client._imp)

	if errCode != 0 {
		if errCode == C.RET_INVALID_KEY_ERR {
			err = ErrMalformedKey
		} else {
			err = networkError(errorMessage[errCode])
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
	client.lock()
	defer client.unlock()

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

	errCode := C.client_get(client._imp, &cKeys[0], &cKeyLens[0], cNKeys, &rst, &n)
	defer C.client_destroy_retrieval_result(client._imp)

	switch errCode {
	case 0:
		err = nil
	case C.RET_INVALID_KEY_ERR:
		err = ErrMalformedKey
	default:
		err = networkError(errorMessage[errCode])
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
	client.lock()
	defer client.unlock()

	rawKey := client.addPrefix(key)

	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cExptime := C.exptime_t(expiration)
	cNoreply := C.bool(client.noreply)

	var rst **C.message_result_t
	var n C.size_t

	errCode := C.client_touch(
		client._imp, &cKey, &cKeyLen, cExptime, cNoreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(client._imp)

	switch errCode {
	case 0:
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
	return networkError(errorMessage[errCode])
}

func (client *Client) incrOrDecr(cmd string, key string, delta uint64) (uint64, error) {
	client.lock()
	defer client.unlock()

	rawKey := client.addPrefix(key)
	cKey := C.CString(rawKey)
	defer C.free(unsafe.Pointer(cKey))
	cKeyLen := C.size_t(len(rawKey))
	cDelta := C.uint64_t(delta)
	cNoreply := C.bool(client.noreply)

	var rst *C.unsigned_result_t
	var n C.size_t

	var errCode C.int

	switch cmd {
	case "incr":
		errCode = C.client_incr(
			client._imp, cKey, cKeyLen, cDelta, cNoreply,
			&rst, &n,
		)
	case "decr":
		errCode = C.client_decr(
			client._imp, cKey, cKeyLen, cDelta, cNoreply,
			&rst, &n,
		)

	}

	defer C.client_destroy_unsigned_result(client._imp)

	if errCode == 0 {
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

	return 0, networkError(errorMessage[errCode])
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
	client.lock()
	defer client.unlock()

	var rst *C.broadcast_result_t
	var n C.size_t
	rv := make(map[string]string)

	errCode := C.client_version(client._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(client._imp)
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

	if errCode != 0 {
		return rv, networkError(errorMessage[errCode])
	}

	return rv, nil
}

// Stats will return a map reflecting stats map of each memcached server
func (client *Client) Stats() (map[string](map[string]string), error) {
	client.lock()
	defer client.unlock()

	var rst *C.broadcast_result_t
	var n C.size_t

	errCode := C.client_stats(client._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(client._imp)

	rv := make(map[string](map[string]string))

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

	if errCode != 0 {
		return rv, networkError(errorMessage[errCode])
	}

	return rv, nil
}

// Quit will close the sockets to each memcached server
func (client *Client) Quit() error {
	client.lock()
	defer client.unlock()

	errCode := C.client_quit(client._imp)
	C.client_destroy_broadcast_result(client._imp)

	if errCode == C.RET_CONN_POLL_ERR || errCode == C.RET_RECV_ERR {
		return nil
	}
	return networkError(errorMessage[errCode])
}
