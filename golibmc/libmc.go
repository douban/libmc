package libmc

/*
#cgo  CFLAGS: -I ${SRCDIR}/../include
#cgo LDFLAGS: -L ${SRCDIR}/../build -l mc -l stdc++
#include "c_client.h"

*/
import "C"
import (
	"errors"
	"fmt"
	"runtime"
	"strconv"
	"strings"
	"unsafe"
)

const (
	MC_HASH_MD5      = 0
	MC_HASH_FNV1_32  = 1
	MC_HASH_FNV1A_32 = 2
	MC_HASH_CRC_32   = 3
)
const MC_DEFAULT_PORT = 11211

type Client struct {
	_imp    unsafe.Pointer
	servers []string
	prefix  string
	noreply bool
}

// credits to bradfitz/gomemcache
// Item is an item to be got or stored in a memcached server.
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
	// Zero means the Item has no expiration time.
	Expiration int64

	// Compare and swap ID.
	casid uint64
}

func New(servers []string, noreply bool, prefix string, hash_fn string, failover bool) (self *Client) {
	self = new(Client)
	// TODO handle hash_fn
	self._imp = C.client_create()

	n := len(servers)
	c_hosts := make([]*C.char, n)
	c_ports := make([]C.uint32_t, n)
	c_aliases := make([]*C.char, n)

	for i, srv := range servers {
		addr_alias := strings.Split(srv, " ")

		addr := addr_alias[0]
		if len(addr_alias) == 2 {
			c_alias := C.CString(addr_alias[1])
			defer C.free(unsafe.Pointer(c_alias))
			c_aliases[i] = c_alias
		}

		host_port := strings.Split(addr, ":")
		host := host_port[0]
		var c_host *C.char = C.CString(host)
		defer C.free(unsafe.Pointer(c_host))
		c_hosts[i] = c_host

		if len(host_port) == 2 {
			port, err := strconv.Atoi(host_port[1])
			if err != nil {
				fmt.Println(err) // TODO handle error
			}
			c_ports[i] = C.uint32_t(port)
		} else {
			c_ports[i] = C.uint32_t(MC_DEFAULT_PORT)
		}
	}

	failoverInt := 0
	if failover {
		failoverInt = 1
	}

	C.client_init(
		self._imp,
		(**C.char)(unsafe.Pointer(&c_hosts[0])),
		(*C.uint32_t)(unsafe.Pointer(&c_ports[0])),
		C.size_t(n),
		(**C.char)(unsafe.Pointer(&c_aliases[0])),
		C.int(failoverInt),
	)

	self.prefix = prefix
	self.noreply = noreply
	runtime.SetFinalizer(self, finalizer)
	return
}

func finalizer(self *Client) {
	C.client_destroy(self._imp)
}

// TODO
func (self *Client) GetHostByKey(key string) string {
	return ""
}

func (self *Client) removePrefix(key string) string {
	if len(self.prefix) == 0 {
		return key
	}
	if strings.HasPrefix(key, "?") {
		return strings.Join([]string{"?", strings.Replace(key[1:], self.prefix, "", 1)}, "")
	} else {
		return strings.Replace(key, self.prefix, "", 1)
	}
}

func (self *Client) addPrefix(key string) string {
	if len(self.prefix) == 0 {
		return key
	}
	if strings.HasPrefix(key, "?") {
		return strings.Join([]string{"?", self.prefix, key[1:]}, "")
	} else {
		return strings.Join([]string{self.prefix, key}, "")
	}
}

// TODO
func (self *Client) Add(item *Item) error {
	return nil
}

// TODO
func (self *Client) Replace(item *Item) error {
	return nil
}

// TODO
func (self *Client) Prepend(item *Item) error {
	return nil
}

// TODO
func (self *Client) Append(item *Item) error {
	return nil
}

func (self *Client) Set(item *Item) error {
	c_key := C.CString(item.Key)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(item.Key))
	c_flags := C.flags_t(item.Flags)
	c_exptime := C.exptime_t(item.Expiration)
	c_noreply := C.bool(self.noreply)
	c_value := C.CString(string(item.Value))
	defer C.free(unsafe.Pointer(c_value))
	c_valueSize := C.size_t(len(item.Value))

	var rst **C.message_result_t
	var n C.size_t

	err_code := C.client_set(
		self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
		c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(self._imp)

	if err_code != 0 {
		return errors.New(strconv.Itoa(int(err_code)))
	}

	// assert n == 1 TODO parse message
	return nil
}

// TODO
func (self *Client) SetMulti(items []*Item) ([]string, error) {
	return []string{}, nil
}

// TODO CompareAndSwap
func (self *Client) Cas(item *Item) error {
	return nil
}

func (self *Client) Delete(key string) error {
	c_key := C.CString(key)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(key))
	c_noreply := C.bool(self.noreply)

	var rst **C.message_result_t
	var n C.size_t

	err_code := C.client_delete(
		self._imp, &c_key, &c_keyLen, c_noreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(self._imp)

	if err_code != 0 {
		return errors.New(strconv.Itoa(int(err_code)))
	}

	// assert n == 1 TODO parse message
	return nil
}

// TODO
func (self *Client) DeleteMulti(keys []string) ([]string, error) {
	return []string{}, nil
}

func (self *Client) Get(key string) (*Item, error) {
	raw_key := self.addPrefix(key)
	c_key := C.CString(key)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(raw_key))
	var rst **C.retrieval_result_t
	var n C.size_t

	err_code := C.client_get(self._imp, &c_key, &c_keyLen, 1, &rst, &n)
	defer C.client_destroy_retrieval_result(self._imp)

	if err_code != 0 {
		return nil, errors.New(strconv.Itoa(int(err_code)))
	}

	if int(n) == 0 {
		return nil, nil
	}

	data_block := C.GoBytes(unsafe.Pointer((*rst).data_block), C.int((*rst).bytes))
	flags := uint32((*rst).flags)
	return &Item{Key: key, Value: data_block, Flags: flags}, nil
}

func (self *Client) GetMulti(keys []string) (map[string]*Item, error) {
	nKeys := len(keys)
	raw_keys := make([]string, len(keys))
	for i, key := range keys {
		raw_keys[i] = self.addPrefix(key)
	}

	c_keys := make([]*C.char, nKeys)
	c_keyLens := make([]C.size_t, nKeys)
	c_nKeys := C.size_t(nKeys)

	for i, raw_key := range raw_keys {
		c_key := C.CString(raw_key)
		defer C.free(unsafe.Pointer(c_key))
		c_keyLen := C.size_t(len(raw_key))
		c_keys[i] = c_key
		c_keyLens[i] = c_keyLen
	}

	var rst **C.retrieval_result_t
	var n C.size_t

	err_code := C.client_get(self._imp, &c_keys[0], &c_keyLens[0], c_nKeys, &rst, &n)
	defer C.client_destroy_retrieval_result(self._imp)

	rv := map[string]*Item{}

	if err_code != 0 {
		return rv, errors.New(strconv.Itoa(int(err_code)))
	}

	if int(n) == 0 {
		return rv, nil
	}

	sr := unsafe.Sizeof(*rst)

	for i := 0; i < int(n); i += 1 {
		raw_key := C.GoStringN((*rst).key, C.int((*rst).key_len))
		data_block := C.GoBytes(unsafe.Pointer((*rst).data_block), C.int((*rst).bytes))
		flags := uint32((*rst).flags)
		key := self.removePrefix(raw_key)
		rv[key] = &Item{Key: key, Value: data_block, Flags: flags}
		rst = (**C.retrieval_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	return rv, nil
}

// TODO
func (self *Client) Gets(key string) (*Item, error) {
	return nil, nil
}

// TODO
func (self *Client) Touch(key string, expiration int64) error {
	return nil
}

// TODO
func (self *Client) Incr(key string, delta uint64) (uint64, error) {
	return 0, nil
}

// TODO
func (self *Client) Decr(key string, delta uint64) (uint64, error) {
	return 0, nil
}

func (self *Client) Version() (map[string]string, error) {
	var rst *C.broadcast_result_t
	var n C.size_t
	rv := make(map[string]string)

	err_code := C.client_version(self._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(self._imp)
	sr := unsafe.Sizeof(*rst)

	for i := 0; i < int(n); i += 1 {
		if rst.lines == nil || rst.line_lens == nil {
			continue
		}

		host := C.GoString(rst.host)
		version := C.GoStringN(*rst.lines, C.int(*rst.line_lens))
		rv[host] = version
		rst = (*C.broadcast_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	if err_code != 0 {
		return rv, errors.New(strconv.Itoa(int(err_code)))
	}

	return rv, nil
}
