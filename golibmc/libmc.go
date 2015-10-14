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
	MC_POLL_TIMEOUT    = C.CFG_POLL_TIMEOUT
	MC_CONNECT_TIMEOUT = C.CFG_CONNECT_TIMEOUT
	MC_RETRY_TIMEOUT   = C.CFG_RETRY_TIMEOUT
	MC_HASH_FUNCTION   = C.CFG_HASH_FUNCTION
)

const (
	MC_HASH_MD5 = iota
	MC_HASH_FNV1_32
	MC_HASH_FNV1A_32
	MC_HASH_CRC_32
)

var hashFunctionMapping = map[int]C.hash_function_options_t{
	MC_HASH_MD5:      C.OPT_HASH_MD5,
	MC_HASH_FNV1_32:  C.OPT_HASH_FNV1_32,
	MC_HASH_FNV1A_32: C.OPT_HASH_FNV1A_32,
	MC_HASH_CRC_32:   C.OPT_HASH_CRC_32,
}

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

func New(servers []string, noreply bool, prefix string, hash_fn int, failover bool) (self *Client) {
	self = new(Client)
	self._imp = C.client_create()
	runtime.SetFinalizer(self, finalizer)

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
				fmt.Errorf("[%s]: %s", srv, err)
				return nil
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

	self.Config(MC_HASH_FUNCTION, int(hashFunctionMapping[hash_fn]))
	self.prefix = prefix
	self.noreply = noreply
	return
}

func finalizer(self *Client) {
	C.client_destroy(self._imp)
}

func (self *Client) Config(opt C.config_options_t, val int) {
	C.client_config(self._imp, opt, C.int(val))
}

func (self *Client) GetServerAddressByKey(key string) string {
	rawKey := self.addPrefix(key)

	c_key := C.CString(rawKey)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(rawKey))
	c_server_addr := C.client_get_server_address_by_key(self._imp, c_key, c_keyLen)
	return C.GoString(c_server_addr)
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

func (self *Client) store(cmd string, item *Item) error {
	key := self.addPrefix(item.Key)

	c_key := C.CString(key)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(key))
	c_flags := C.flags_t(item.Flags)
	c_exptime := C.exptime_t(item.Expiration)
	c_noreply := C.bool(self.noreply)
	c_value := C.CString(string(item.Value))
	defer C.free(unsafe.Pointer(c_value))
	c_valueSize := C.size_t(len(item.Value))

	var rst **C.message_result_t
	var n C.size_t

	var err_code C.int
	switch cmd {
	case "set":
		err_code = C.client_set(
			self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
			c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
		)
	case "add":
		err_code = C.client_add(
			self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
			c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
		)
	case "replace":
		err_code = C.client_replace(
			self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
			c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
		)
	case "prepend":
		err_code = C.client_prepend(
			self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
			c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
		)
	case "append":
		err_code = C.client_prepend(
			self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
			c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
		)
	case "cas":
		err_code = C.client_cas(
			self._imp, &c_key, &c_keyLen, &c_flags, c_exptime, nil,
			c_noreply, &c_value, &c_valueSize, 1, &rst, &n,
		)
	}
	defer C.client_destroy_message_result(self._imp)

	if err_code == 0 {
		if self.noreply {
			return nil
		} else if int(n) == 1 && (*rst).type_ == C.MSG_STORED {
			return nil
		}
	}

	return errors.New(strconv.Itoa(int(err_code)))
}

func (self *Client) Add(item *Item) error {
	return self.store("add", item)
}

func (self *Client) Replace(item *Item) error {
	return self.store("replace", item)
}

func (self *Client) Prepend(item *Item) error {
	return self.store("prepend", item)
}

func (self *Client) Append(item *Item) error {
	return self.store("append", item)
}

func (self *Client) Set(item *Item) error {
	return self.store("set", item)
}

func (self *Client) SetMulti(items []*Item) ([]string, error) {
	nItems := len(items)
	c_keys := make([]*C.char, nItems)
	c_keyLens := make([]C.size_t, nItems)
	c_values := make([]*C.char, nItems)
	c_valueSizes := make([]C.size_t, nItems)
	c_flagsList := make([]C.flags_t, nItems)

	for i, item := range items {
		rawKey := self.addPrefix(item.Key)

		c_key := C.CString(rawKey)
		defer C.free(unsafe.Pointer(c_key))
		c_keys[i] = c_key

		c_keyLen := C.size_t(len(rawKey))
		c_keyLens[i] = c_keyLen

		c_val := C.CString(string(item.Value))
		defer C.free(unsafe.Pointer(c_val))
		c_values[i] = c_val

		c_valueSize := C.size_t(len(item.Value))
		c_valueSizes[i] = c_valueSize

		c_flags := C.flags_t(item.Flags)
		c_flagsList[i] = c_flags
	}

	c_exptime := C.exptime_t(items[0].Expiration)
	c_noreply := C.bool(self.noreply)
	c_nItems := C.size_t(nItems)

	var results **C.message_result_t
	var n C.size_t

	err_code := C.client_set(
		self._imp,
		(**C.char)(&c_keys[0]),
		(*C.size_t)(&c_keyLens[0]),
		(*C.flags_t)(&c_flagsList[0]),
		c_exptime,
		nil,
		c_noreply,
		(**C.char)(&c_values[0]),
		(*C.size_t)(&c_valueSizes[0]),
		c_nItems,
		&results, &n,
	)
	defer C.client_destroy_message_result(self._imp)
	if err_code == 0 {
		return []string{}, nil
	}
	err := errors.New(strconv.Itoa(int(err_code)))

	sr := unsafe.Sizeof(*results)
	storedKeySet := make(map[string]struct{})
	for i := 0; i <= int(n); i += 1 {
		if (*results).type_ == C.MSG_STORED {
			storedKey := C.GoStringN((*results).key, C.int((*results).key_len))
			storedKeySet[storedKey] = struct{}{}
		}

		results = (**C.message_result_t)(
			unsafe.Pointer(uintptr(unsafe.Pointer(results)) + sr),
		)
	}
	failedKeys := make([]string, len(items)-len(storedKeySet))
	i := 0
	for _, item := range items {
		if _, contains := storedKeySet[item.Key]; contains {
			continue
		}
		failedKeys[i] = self.removePrefix(item.Key)
		i += 1
	}
	return failedKeys, err
}

func (self *Client) Cas(item *Item) error {
	return self.store("cas", item)
}

func (self *Client) Delete(key string) error {
	rawKey := self.addPrefix(key)

	c_key := C.CString(rawKey)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(rawKey))
	c_noreply := C.bool(self.noreply)

	var rst **C.message_result_t
	var n C.size_t

	err_code := C.client_delete(
		self._imp, &c_key, &c_keyLen, c_noreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(self._imp)

	if err_code == 0 {
		if self.noreply {
			return nil
		} else if int(n) == 1 && ((*rst).type_ == C.MSG_DELETED || (*rst).type_ == C.MSG_NOT_FOUND) {
			return nil
		}
	}

	return errors.New(strconv.Itoa(int(err_code)))
}

func (self *Client) DeleteMulti(keys []string) (failedKeys []string, err error) {
	var rawKeys []string
	if len(self.prefix) == 0 {
		rawKeys = keys
	} else {
		rawKeys = make([]string, len(keys))
		for i, key := range keys {
			rawKeys[i] = key
		}
	}

	nKeys := len(rawKeys)
	c_nKeys := C.size_t(nKeys)
	c_keys := make([]*C.char, nKeys)
	c_keyLens := make([]C.size_t, nKeys)
	c_noreply := C.bool(self.noreply)

	var results **C.message_result_t
	var n C.size_t

	for i, key := range rawKeys {
		c_key := C.CString(key)
		defer C.free(unsafe.Pointer(c_key))
		c_keys[i] = c_key

		c_keyLen := C.size_t(len(key))
		c_keyLens[i] = c_keyLen
	}
	err_code := C.client_delete(
		self._imp, (**C.char)(&c_keys[0]), (*C.size_t)(&c_keyLens[0]), c_noreply, c_nKeys,
		&results,
		&n,
	)
	defer C.client_destroy_message_result(self._imp)

	if err_code == 0 {
		return
	}

	if self.noreply {
		failedKeys = keys
		return
	}

	deletedKeySet := make(map[string]struct{})
	sr := unsafe.Sizeof(*results)
	for i := 0; i <= int(n); i += 1 {
		if (*results).type_ == C.MSG_DELETED {
			deletedKey := C.GoStringN((*results).key, C.int((*results).key_len))
			deletedKeySet[deletedKey] = struct{}{}
		}

		results = (**C.message_result_t)(
			unsafe.Pointer(uintptr(unsafe.Pointer(results)) + sr),
		)
	}
	err = errors.New(strconv.Itoa(int(err_code)))
	failedKeys = make([]string, len(rawKeys)-len(deletedKeySet))
	i := 0
	for _, key := range rawKeys {
		if _, contains := deletedKeySet[key]; contains {
			continue
		}
		failedKeys[i] = self.removePrefix(key)
		i += 1
	}
	return
}

func (self *Client) getOrGets(cmd string, key string) (item *Item, err error) {
	rawKey := self.addPrefix(key)

	c_key := C.CString(rawKey)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(rawKey))
	var rst **C.retrieval_result_t
	var n C.size_t

	var err_code C.int
	switch cmd {
	case "get":
		err_code = C.client_get(self._imp, &c_key, &c_keyLen, 1, &rst, &n)
	case "gets":
		err_code = C.client_gets(self._imp, &c_key, &c_keyLen, 1, &rst, &n)
	}

	defer C.client_destroy_retrieval_result(self._imp)

	if err_code != 0 {
		err = errors.New(strconv.Itoa(int(err_code)))
		return
	}

	if int(n) == 0 {
		return
	}

	data_block := C.GoBytes(unsafe.Pointer((*rst).data_block), C.int((*rst).bytes))
	flags := uint32((*rst).flags)
	if cmd == "get" {
		item = &Item{Key: key, Value: data_block, Flags: flags}
		return
	}

	casid := uint64((*rst).cas_unique)
	item = &Item{Key: key, Value: data_block, Flags: flags, casid: casid}
	return
}

func (self *Client) Get(key string) (*Item, error) {
	return self.getOrGets("get", key)
}

func (self *Client) Gets(key string) (*Item, error) {
	return self.getOrGets("gets", key)
}

func (self *Client) GetMulti(keys []string) (rv map[string]*Item, err error) {
	nKeys := len(keys)
	var rawKeys []string
	if len(self.prefix) == 0 {
		rawKeys = keys
	} else {
		rawKeys = make([]string, len(keys))
		for i, key := range keys {
			rawKeys[i] = self.addPrefix(key)
		}
	}

	c_keys := make([]*C.char, nKeys)
	c_keyLens := make([]C.size_t, nKeys)
	c_nKeys := C.size_t(nKeys)

	for i, raw_key := range rawKeys {
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

	if err_code != 0 {
		err = errors.New(strconv.Itoa(int(err_code)))
		return
	}

	if int(n) == 0 {
		return
	}

	sr := unsafe.Sizeof(*rst)
	rv = make(map[string]*Item, int(n))
	for i := 0; i < int(n); i += 1 {
		raw_key := C.GoStringN((*rst).key, C.int((*rst).key_len))
		data_block := C.GoBytes(unsafe.Pointer((*rst).data_block), C.int((*rst).bytes))
		flags := uint32((*rst).flags)
		key := self.removePrefix(raw_key)
		rv[key] = &Item{Key: key, Value: data_block, Flags: flags}
		rst = (**C.retrieval_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	return
}

func (self *Client) Touch(key string, expiration int64) error {
	rawKey := self.addPrefix(key)

	c_key := C.CString(rawKey)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(rawKey))
	c_exptime := C.exptime_t(expiration)
	c_noreply := C.bool(self.noreply)

	var rst **C.message_result_t
	var n C.size_t

	err_code := C.client_touch(
		self._imp, &c_key, &c_keyLen, c_exptime, c_noreply, 1, &rst, &n,
	)
	defer C.client_destroy_message_result(self._imp)

	if err_code == 0 {
		if self.noreply {
			return nil
		} else if int(n) == 1 && (*rst).type_ == C.MSG_TOUCHED {
			return nil
		}
	}
	return errors.New(strconv.Itoa(int(err_code)))
}

func (self *Client) incrOrDecr(cmd string, key string, delta uint64) (uint64, error) {
	rawKey := self.addPrefix(key)
	c_key := C.CString(rawKey)
	defer C.free(unsafe.Pointer(c_key))
	c_keyLen := C.size_t(len(rawKey))
	c_delta := C.uint64_t(delta)
	c_noreply := C.bool(self.noreply)

	var rst *C.unsigned_result_t
	var n C.size_t

	var err_code C.int

	switch cmd {
	case "incr":
		err_code = C.client_incr(
			self._imp, c_key, c_keyLen, c_delta, c_noreply,
			&rst, &n,
		)
	case "decr":
		err_code = C.client_decr(
			self._imp, c_key, c_keyLen, c_delta, c_noreply,
			&rst, &n,
		)

	}

	defer C.client_destroy_unsigned_result(self._imp)

	if err_code == 0 {
		if self.noreply {
			return 0, nil
		} else if int(n) == 1 && rst != nil {
			return uint64(rst.value), nil
		}
	}
	return 0, errors.New(strconv.Itoa(int(err_code)))
}

func (self *Client) Incr(key string, delta uint64) (uint64, error) {
	return self.incrOrDecr("incr", key, delta)
}

func (self *Client) Decr(key string, delta uint64) (uint64, error) {
	return self.incrOrDecr("decr", key, delta)
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

func (self *Client) Stats() (map[string](map[string]string), error) {
	var rst *C.broadcast_result_t
	var n C.size_t

	err_code := C.client_stats(self._imp, &rst, &n)
	defer C.client_destroy_broadcast_result(self._imp)

	rv := make(map[string](map[string]string))

	sr := unsafe.Sizeof(*rst)

	for i := 0; i < int(n); i += 1 {
		if rst.lines == nil || rst.line_lens == nil {
			continue
		}
		host := C.GoString(rst.host)
		nMetrics := int(rst.len)

		rv[host] = make(map[string]string)

		var c_lines **C.char = rst.lines
		var c_lineLens *C.size_t = rst.line_lens

		sLine := unsafe.Sizeof(*c_lines)
		sLineLen := unsafe.Sizeof(*c_lineLens)
		for j := 0; j < nMetrics; j += 1 {
			metricLine := C.GoStringN(*c_lines, C.int(*c_lineLens))
			kv := strings.SplitN(metricLine, " ", 2)
			rv[host][kv[0]] = kv[1]
			c_lines = (**C.char)(unsafe.Pointer(uintptr(unsafe.Pointer(c_lines)) + sLine))
			c_lineLens = (*C.size_t)(unsafe.Pointer(uintptr(unsafe.Pointer(c_lineLens)) + sLineLen))
		}

		rst = (*C.broadcast_result_t)(unsafe.Pointer(uintptr(unsafe.Pointer(rst)) + sr))
	}

	if err_code != 0 {
		return rv, errors.New(strconv.Itoa(int(err_code)))
	}

	return rv, nil
}

func (self *Client) Quit() error {
	err_code := C.client_quit(self._imp)
	defer C.client_destroy_broadcast_result(self._imp)

	if err_code == 0 {
		return nil
	}
	return errors.New(strconv.Itoa(int(err_code)))
}
