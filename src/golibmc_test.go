package golibmc

import (
	"fmt"
	"strconv"
	"strings"
	"testing"
	"time"
)

const LocalMC = "localhost:21211"
const ErrorSet = "Error on Set"
const ErrorGetAfterSet = "Error on Get after set"

var Prefixes = []string{"", "prefix"}

type TestFunc func(*Client, *testing.T)

func testNormalCommand(t *testing.T, fn TestFunc) {
	for _, nServers := range []int{1, 2, 10} {
		for _, prefix := range Prefixes {
			mc := newSimplePrefixClient(nServers, prefix)
			fn(mc, t)
		}
	}
}

func newSimpleClient(n int) *Client {
	return newSimplePrefixClient(n, "")
}

func newSimpleNoreplyClient(n int) *Client {
	servers := make([]string, n)
	for i := 0; i < n; i++ {
		servers[i] = fmt.Sprintf("localhost:%d", 21211+i)
	}
	noreply := true
	prefix := ""
	hashFunc := HashCRC32
	failover := false
	disableLock := false

	return New(servers, noreply, prefix, hashFunc, failover, disableLock)
}

func newSimplePrefixClient(n int, prefix string) *Client {
	servers := make([]string, n)
	for i := 0; i < n; i++ {
		servers[i] = fmt.Sprintf("localhost:%d", 21211+i)
	}
	noreply := false
	hashFunc := HashCRC32
	failover := false
	disableLock := false

	return New(servers, noreply, prefix, hashFunc, failover, disableLock)
}

func TestInputServer(t *testing.T) {
	servers := []string{"localhost:invalid_port"}
	noreply := false
	prefix := ""
	hashFunc := HashCRC32
	failover := false
	disableLock := false

	mc := New(servers, noreply, prefix, hashFunc, failover, disableLock)
	cn, err := mc.newConn()
	if err == nil {
		t.Error(cn)
	}
}

func TestStats(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimplePrefixClient(nServers, "")
		testStats(mc, t)
	}
}

func testStats(mc *Client, t *testing.T) {
	statsDict, err := mc.Stats()
	if !(err == nil && len(statsDict) == len(mc.servers) && len(mc.servers) > 0) {
		t.Errorf("%d %d", len(statsDict), len(mc.servers))
	}

	if m, _ := statsDict[LocalMC]["version"]; len(m) == 0 {
		t.Error(err)
	}
}

func TestPrefix(t *testing.T) {
	c1 := Client{}
	testPrefix := "prefix"
	c2 := Client{prefix: testPrefix}
	key := "foo"

	if x := c1.addPrefix(key); x != key {
		t.Error(x)
	}
	if x := c1.removePrefix(key); x != key {
		t.Error(x)
	}

	key2 := strings.Join([]string{key, c2.prefix}, "")
	for _, key := range []string{key, key2} {

		prefixedKey := c2.addPrefix(key)
		if x := strings.Join([]string{c2.prefix, key}, ""); x != prefixedKey {
			t.Error(x)
		}
		if x := c2.removePrefix(prefixedKey); x != key {
			t.Error(x)
		}
	}

	keyTmpls := []string{
		"%sforever/young",
		"forever%s/young",
		"forever/young/%s",
	}

	for _, keyTmpl := range keyTmpls {
		key := fmt.Sprintf(keyTmpl, c2.prefix)
		if x := c2.removePrefix(c2.addPrefix(key)); key != x {
			t.Error(x)
		}
	}
}

func TestGetServerAddress(t *testing.T) {
	mc := newSimplePrefixClient(1, "")
	if addr := mc.GetServerAddressByKey("key"); addr != LocalMC {
		t.Error(addr)
	}

	// test basic ketama
	servers := []string{"localhost", "myhost:11211", "127.0.0.1:11212", "myhost:11213"}
	rs := map[string]string{
		"test:10000": "localhost:11211",
		"test:20000": "127.0.0.1:11212",
		"test:30000": "127.0.0.1:11212",
		"test:40000": "127.0.0.1:11212",
		"test:50000": "127.0.0.1:11212",
		"test:60000": "myhost:11213",
		"test:70000": "127.0.0.1:11212",
		"test:80000": "127.0.0.1:11212",
		"test:90000": "127.0.0.1:11212",
	}
	testRouter(t, servers, rs, "")

	// test aliased ketama
	servers = []string{
		"192.168.1.211:11211 tango.mc.douban.com",
		"192.168.1.212:11212 uniform.mc.douban.com",
		"192.168.1.211:11212 victor.mc.douban.com",
		"192.168.1.212:11211 whiskey.mc.douban.com",
	}
	rs = map[string]string{
		"test:10000": "whiskey.mc.douban.com",
		"test:20000": "victor.mc.douban.com",
		"test:30000": "victor.mc.douban.com",
		"test:40000": "victor.mc.douban.com",
		"test:50000": "victor.mc.douban.com",
		"test:60000": "uniform.mc.douban.com",
		"test:70000": "tango.mc.douban.com",
		"test:80000": "victor.mc.douban.com",
		"test:90000": "victor.mc.douban.com",
	}
	testRouter(t, servers, rs, "")

	// test prefixed ketama
	servers = []string{
		"localhost", "myhost:11211", "127.0.0.1:11212", "myhost:11213",
	}
	rs = map[string]string{
		"test:10000": "127.0.0.1:11212",
		"test:20000": "localhost:11211",
		"test:30000": "myhost:11213",
		"test:40000": "myhost:11211",
		"test:50000": "myhost:11213",
		"test:60000": "myhost:11213",
		"test:70000": "localhost:11211",
		"test:80000": "myhost:11213",
		"test:90000": "myhost:11213",
	}
	prefix := "/prefix"
	testRouter(t, servers, rs, prefix)
}

func testRouter(t *testing.T, servers []string, rs map[string]string, prefix string) {
	noreply := false
	hashFunc := HashMD5
	failover := false
	disableLock := false

	mc := New(servers, noreply, prefix, hashFunc, failover, disableLock)
	for key, expectedServerAddr := range rs {
		actualServerAddr := mc.GetServerAddressByKey(key)
		if actualServerAddr != expectedServerAddr {
			t.Errorf(
				"actualServerAddr: %s, expectedServerAddr: %s",
				actualServerAddr,
				expectedServerAddr,
			)
		}
	}
}

func TestSetNGet(t *testing.T) {
	testNormalCommand(t, testSetNGet)
}

func testSetNGet(mc *Client, t *testing.T) {
	key := "google谷歌"
	value := "Google"
	version, err := mc.Version()
	if len(version) == 0 || err != nil {
		t.Error("Bad version, make sure memcached servers are started")
	}

	item := Item{
		Key:        key,
		Value:      []byte(value),
		Flags:      0,
		Expiration: 0,
	}
	err = mc.Set(&item)
	if err != nil {
		t.Error(ErrorSet)
	}

	val, err := mc.Get(key)
	if v := string((*val).Value); v != value || err != nil {
		t.Error(ErrorGetAfterSet)
	}

	dct, err := mc.GetMulti([]string{key})
	if err != nil {
		t.Error(ErrorGetAfterSet)
	}

	if len(dct) != 1 {
		t.Error(ErrorGetAfterSet)
	}

	if v := string((*dct[key]).Value); v != value {
		t.Error(ErrorGetAfterSet)
	}

	err = mc.Delete(key)
	if err != nil {
		t.Error(ErrorGetAfterSet)
	}

	val, err = mc.Get(key)
	if !(val == nil && err == ErrCacheMiss) {
		t.Error(ErrorGetAfterSet)
	}

	dct, err = mc.GetMulti([]string{key})
	if !(len(dct) == 0 && err == ErrCacheMiss) {
		t.Error(ErrorGetAfterSet)
	}
}

func TestSetMultiNGetMulti(t *testing.T) {
	testNormalCommand(t, testSetMultiNGetMulti)
}

func testSetMultiNGetMulti(mc *Client, t *testing.T) {
	// Init and SetMuti Values
	nItems := 2
	items := make([]*Item, nItems)
	keys := make([]string, nItems)

	for i := 0; i < nItems; i++ {
		key := fmt.Sprintf("test_multi_key_%d", i)
		value := fmt.Sprintf("v%d", i)
		items[i] = &Item{
			Key:        key,
			Value:      []byte(value),
			Flags:      0,
			Expiration: 0,
		}
		keys[i] = key
	}

	mc.SetMulti(items)

	// GetMulti Values and Check
	itemsMap, _ := mc.GetMulti(keys)

	for _, item := range items {
		itemGot := itemsMap[item.Key]
		if itemGot == nil || string(itemGot.Value) != string(item.Value) {
			t.Error(ErrorGetAfterSet)
		}
	}

	// Modify Values and Check Again
	for i := 0; i < len(keys); i++ {
		newValue := fmt.Sprintf("V%d", i)
		items[i].Value = []byte(newValue)
	}

	mc.SetMulti(items)
	itemsMap, _ = mc.GetMulti(keys)

	for _, item := range items {
		itemGot := itemsMap[item.Key]
		if itemGot == nil || string(itemGot.Value) != string(item.Value) {
			t.Error(ErrorGetAfterSet)
		}

		if x := string(itemGot.Value); !strings.HasPrefix(x, "V") {
			t.Error(x)
		}
	}
	// Delete Values and Check Again
	mc.DeleteMulti(keys)
	itemsMap, err := mc.GetMulti(keys)
	if !(len(itemsMap) == 0 && err == ErrCacheMiss) {
		t.Error(err)
	}
}

func TestIncrNDecr(t *testing.T) {
	testNormalCommand(t, testIncrNDecr)
}

func testIncrNDecr(mc *Client, t *testing.T) {
	key := "test_incr_decr"
	item := &Item{
		Key:        key,
		Value:      []byte("99"),
		Flags:      0,
		Expiration: 0,
	}
	mc.Set(item)
	if val, err := mc.Incr(key, 1); !(val == 100 && err == nil) {
		t.Error(err)
	}

	if val, err := mc.Decr(key, 1); !(val == 99 && err == nil) {
		t.Error(err)
	}

	mc.Delete(key)

	if val, err := mc.Incr(key, 1); !(val == 0 && err != nil) {
		t.Error(err)
	}

	if val, err := mc.Decr(key, 1); !(val == 0 && err != nil) {
		t.Error(err)
	}
}

func TestLargeValue(t *testing.T) {
	testNormalCommand(t, testLargeValue)
}

func testLargeValue(mc *Client, t *testing.T) {
	key := "test_incr_decr"
	item := &Item{
		Key:        key,
		Value:      make([]byte, 1*1024*1024),
		Flags:      0,
		Expiration: 0,
	}
	if err := mc.Set(item); err == nil {
		t.Error(err)
	}
}

func TestCasAndGets(t *testing.T) {
	testNormalCommand(t, testCasAndGets)
}

func testCasAndGets(mc *Client, t *testing.T) {
	key := "test_cas_and_gets"
	val := []byte("o")
	val2 := []byte("2")
	item := &Item{Key: key, Value: val}
	err := mc.Set(item)
	if err != nil {
		t.Error(err)
	}
	item2, err := mc.Gets(key)
	if item2 == nil || err != nil {
		t.Error(err)
	}
	(*item).Value = val2
	mc.Set(item)
	item3, err := mc.Gets(key)
	if item3 == nil || err != nil {
		t.Error(err)
	}
	if string((*item2).Value) != string(val) ||
		string((*item3).Value) != string(val2) {
		t.Error(err)
	}
	if (*item2).casid == (*item3).casid {
		t.Error(err)
	}

	item3.Value = []byte("VVV")
	err = mc.Cas(item3)
	if err != nil {
		t.Error(err)
	}

	item3.Key = "not_existed_key"
	mc.Delete(item3.Key)
	err = mc.Cas(item3)
	if err != ErrCacheMiss {
		t.Error(err)
	}
}

func TestNoreploy(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleNoreplyClient(nServers)
		testNoreploy(mc, t)
	}
}

func testNoreploy(mc *Client, t *testing.T) {
	key := "google"
	value := "Google"
	item := &Item{
		Key:        key,
		Value:      []byte(value),
		Flags:      0,
		Expiration: 0,
	}
	if err := mc.Set(item); err != nil {
		t.Error(err)
	}

	itemGot, err := mc.Gets(key)
	if err != nil || string(itemGot.Value) != value {
		t.Error(err)
	}
	(*item).casid = itemGot.casid
	value2 := "22"
	(*item).Value = []byte(value2)

	err = mc.Cas(item)
	if err != nil {
		t.Error(err)
	}

	itemGot, err = mc.Get(key)
	if err != nil || string(itemGot.Value) != value2 {
		t.Error(err)
	}

	val3, err := mc.Incr(key, 10)
	if err != nil || val3 != 0 {
		t.Error(val3, err)
	}

	itemGot, err = mc.Gets(key)
	if err != nil || string(itemGot.Value) != "32" {
		t.Error(err)
	}

	val4, err := mc.Decr(key, 12)
	if err != nil || val4 != 0 {
		t.Error(err)
	}

	itemGot, err = mc.Gets(key)
	if err != nil || string(itemGot.Value) != "20" {
		t.Error(err)
	}

	err = mc.Delete(key)
	if err != nil {
		t.Error(err)
	}
}

func TestTouch(t *testing.T) {
	testNormalCommand(t, testTouch)
}

func testTouch(mc *Client, t *testing.T) {
	key := "test"
	value := "1"
	item := &Item{
		Key:   key,
		Value: []byte(value),
	}

	// Touch -1
	mc.Set(item)
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != value {
		t.Error(err)
	}

	if err := mc.Touch(key, -1); err != nil {
		t.Error(err)
	}

	if itemGot, err := mc.Get(key); err != ErrCacheMiss || itemGot != nil {
		t.Error(err)
	}

	// Touch
	mc.Set(item)
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != value {
		t.Error(err)
	}
	if err := mc.Touch(key, 1); err != nil {
		t.Error(err)
	}
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != value {
		t.Error(err)
	}
	// The value is expected to be exired in 1s,
	// so we sleep 2s and check if it's expired.
	time.Sleep(2 * time.Second)
	if itemGot, err := mc.Get(key); err != ErrCacheMiss || itemGot != nil {
		t.Error(err)
	}
}

func TestAdd(t *testing.T) {
	testNormalCommand(t, testAdd)
}

func testAdd(mc *Client, t *testing.T) {
	key := "test_add"
	valueToAdd := "tt"
	item := &Item{
		Key:   key,
		Value: []byte(valueToAdd),
	}
	mc.Delete(key)
	if err := mc.Add(item); err != nil {
		t.Error(err)
	}
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != valueToAdd {
		t.Error(err)
	}
	if err := mc.Add(item); err != ErrNotStored {
		t.Error(err)
	}
	key2 := "test_add2"
	item.Key = key2
	mc.Delete(key2)
	if err := mc.Add(item); err != nil {
		t.Error(err)
	}
}

func TestReplace(t *testing.T) {
	testNormalCommand(t, testReplace)
}

func testReplace(mc *Client, t *testing.T) {
	key := "test_replace"
	item := &Item{
		Key:   key,
		Value: []byte(""),
	}
	mc.Delete(key)

	if err := mc.Replace(item); err != ErrNotStored {
		t.Error(err)
	}
	item.Value = []byte("b")
	if err := mc.Set(item); err != nil {
		t.Error(err)
	}
	item.Value = []byte("a")

	if err := mc.Replace(item); err != nil {
		t.Error(err)
	}

	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != "a" {
		t.Error(err)
	}
}

func TestPrepend(t *testing.T) {
	testNormalCommand(t, testPrepend)
}

func testPrepend(mc *Client, t *testing.T) {
	key := "test_prepend"
	value := "prepend\n"
	value2 := ""
	value3 := "before\n"
	mc.Delete(key)
	item := &Item{
		Key:   key,
		Value: []byte(value),
	}
	if err := mc.Prepend(item); err == nil {
		t.Error(err)
	}

	item.Value = []byte(value2)
	if err := mc.Set(item); err != nil {
		t.Error(err)
	}

	item.Value = []byte(value)
	if err := mc.Prepend(item); err != nil {
		t.Error(err)
	}

	item.Value = []byte(value3)
	if err := mc.Prepend(item); err != nil {
		t.Error(err)
	}

	itemGot, err := mc.Get(key)
	if !(err == nil && string(itemGot.Value) == value3+value) {
		t.Error(err)
	}
}

func TestAppend(t *testing.T) {
	testNormalCommand(t, testAppend)
}

func testAppend(mc *Client, t *testing.T) {
	key := "test_append"
	value := "append\n"
	value2 := ""
	value3 := "after\n"
	mc.Delete(key)
	item := &Item{
		Key:   key,
		Value: []byte(value),
	}
	if err := mc.Append(item); err == nil {
		t.Error(err)
	}

	item.Value = []byte(value2)
	if err := mc.Set(item); err != nil {
		t.Error(err)
	}

	item.Value = []byte(value)
	if err := mc.Append(item); err != nil {
		t.Error(err)
	}

	item.Value = []byte(value3)
	if err := mc.Append(item); err != nil {
		t.Error(err)
	}

	itemGot, err := mc.Get(key)
	if !(err == nil && string(itemGot.Value) == value+value3) {
		_ = itemGot
		t.Errorf("[%s] : [%s]", string(itemGot.Value), value+value3)
	}
}

func TestSpecialItems(t *testing.T) {
	testNormalCommand(t, testSpecialItems)
}

func testSpecialItems(mc *Client, t *testing.T) {
	key := "a\r\nb"
	if err := mc.Delete(key); err == nil {
		t.Error(err)
	}
	value := ""
	item := &Item{
		Key:   key,
		Value: []byte(value),
	}
	if err := mc.Set(item); err == nil {
		t.Error(err)
	}
}

func TestInjection(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimplePrefixClient(nServers, "")
		testInjection(mc, t)
	}
}

func testInjection(mc *Client, t *testing.T) {
	mc.Delete("injected")
	key := strings.Repeat("a", 250)
	invalidKey := strings.Repeat("a", 251)
	value := "biu"
	item := &Item{
		Key:        key,
		Value:      []byte(value),
		Flags:      0,
		Expiration: 0,
	}
	if err := mc.Set(item); err != nil {
		t.Error(err)
	}

	item.Key = invalidKey
	if err := mc.Set(item); err == nil {
		t.Error(err)
	}

	item.Value = []byte("set injected 0 3600 10\r\n1234567890")
	if err := mc.Set(item); err == nil {
		t.Error(err)
	}

	it, err := mc.Get("injected")
	if it != nil || err != ErrCacheMiss {
		t.Error(err)
	}

	item.Key = "key1"
	item.Value = []byte("1234567890")

	if err := mc.Set(item); err != nil {
		t.Error(err)
	}

	item.Key = "key1 0"
	item.Value = []byte("123456789012345678901234567890\r\nset injected 0 3600 3\r\nINJ\r\n")

	if err := mc.Set(item); err == nil {
		t.Error(err)
	}

	item, err = mc.Get("injected")
	if item != nil || err != ErrCacheMiss {
		t.Error(err)
	}
}

func TestMaxiov(t *testing.T) {
	testNormalCommand(t, testMaxiov)
}

func testMaxiov(mc *Client, t *testing.T) {

	keyTmpl := "not_existed.%d"
	nKeys := 1000
	keys := make([]string, nKeys)
	for i := 0; i < nKeys; i++ {
		keys[i] = fmt.Sprintf(keyTmpl, i)
	}

	mc.DeleteMulti(keys)
	itemsGot, err := mc.GetMulti(keys)
	if len(itemsGot) != 0 || err != ErrCacheMiss {
		t.Error(err)
	}
}

func TestQuit(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimplePrefixClient(nServers, "")
		mc2 := newSimplePrefixClient(nServers, "")
		testQuit(mc, mc2, t)
	}
}

func testQuit(mc, mc2 *Client, t *testing.T) {
	// Init and SetMuti Values
	nItems := 1000
	items := make([]*Item, nItems)
	keys := make([]string, nItems)

	for i := 0; i < nItems; i++ {
		key := fmt.Sprintf("test_quit_key_%d", i)
		value := fmt.Sprintf("v%d", i)
		items[i] = &Item{
			Key:        key,
			Value:      []byte(value),
			Flags:      0,
			Expiration: 0,
		}
		keys[i] = key
	}

	mc.SetMulti(items)

	if _, err := mc.DeleteMulti(keys); err != nil && err != ErrCacheMiss {
		t.Error(err)
	}
	if _, err := mc.SetMulti(items); err != nil {
		t.Error(err)
	}
	if _, err := mc2.DeleteMulti(keys); err != nil {
		t.Error(err)
	}
	if _, err := mc2.SetMulti(items); err != nil {
		t.Error(err)
	}
	if versions, err := mc.Version(); err != nil || len(versions) == 0 {
		t.Error(err)
	}
	if versions, err := mc2.Version(); err != nil || len(versions) == 0 {
		t.Error(err)
	}

	st1, err1 := mc.Stats()
	st2, err2 := mc2.Stats()
	if err1 != nil || err2 != nil {
		t.Error(err1, err2)
	}

	nc1, err1 := strconv.Atoi(st1[LocalMC]["curr_connections"])
	nc2, err2 := strconv.Atoi(st2[LocalMC]["curr_connections"])
	if err1 != nil || err2 != nil {
		t.Error(err1, err2)
	}
	if nc1 != nc2 {
		t.Errorf("nc1: %d, nc2: %d", nc1, nc2)
	}

	if err := mc.Quit(); err != nil {
		t.Errorf("Error in Quit: %s", err)
	}

	for i := 0; i < 3; i++ {
		st2, err := mc2.Stats()
		if err != nil {
			t.Error(err)
			break
		}
		nc2, err = strconv.Atoi(st2[LocalMC]["curr_connections"])
		if err != nil {
			t.Error(err)
			break
		}
		if nc1-1 == nc2 {
			break
		} else {
			time.Sleep(time.Second)
		}
	}

	if nc1-1 != nc2 {
		t.Errorf("nc1: %d, nc2: %d", nc1, nc2)
	}

	// back to life immediately
	// itemsGot, err := mc.GetMulti(keys)
	// if err != nil || len(itemsGot) != nItems {
	// 	t.Error(err)
	// }
}

func TestMaybeOpenNewConnections(t *testing.T) {
	mc := newSimpleClient(1)
	mc.SetConnMaxOpen(10)
	mc.SetConnMaxLifetime(1 * time.Second)
	req := make(chan connRequest, 1)
	reqKey := mc.nextRequest
	mc.nextRequest++
	mc.connRequests[reqKey] = req
	mc.maybeOpenNewConnections()

	select {
	case ret, ok := <-req:
		if !ok {
			t.Errorf("Receive an invalid connection: %v", ret.err)
		}
		mc.putConn(ret.conn, ret.err)
	}
	if len(mc.freeConns) != 1 {
		t.Errorf("mc.freeConns' len expected 1 but %d", len(mc.freeConns))
	}
	if mc.numOpen != 1 {
		t.Errorf("mc.numOpen expected 1 but %d", mc.numOpen)
	}

	// Check cleaner
	time.Sleep(2 * time.Second)
	if len(mc.freeConns) != 0 {
		t.Errorf("mc.freeConns' len expected 0 but (%d)", len(mc.freeConns))
	}
	if mc.numOpen != 0 {
		t.Errorf("mc.numOpen expected 0 but (%d)", mc.numOpen)
	}
	mc.Quit()
}

func BenchmarkSetAndGet(b *testing.B) {
	mc := newSimplePrefixClient(1, "")
	key := "google"
	value := "Google"
	item := Item{
		Key:        key,
		Value:      []byte(value),
		Flags:      0,
		Expiration: 0,
	}
	mc.Set(&item)
	for i := 0; i < b.N; i++ {
		mc.Get(key)
	}
}

func TestFlushAll(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimplePrefixClient(nServers, "")
		testFlushAll(mc, t)
	}
}

func testFlushAll(mc *Client, t *testing.T) {
	// Check if flush_all is disabled by default
	flushedHosts, err := mc.FlushAll()
	if !(err != nil && len(flushedHosts) == 0) {
		t.Error(err)
	}
	mc.ToggleFlushAllFeature(true)
	// Make sure we can flush directly
	flushedHosts, err = mc.FlushAll()
	if err != nil || len(flushedHosts) != len(mc.servers) {
		t.Error(err)
	}

	// Set some random data
	nItems := 10
	items := make([]*Item, nItems)
	keys := make([]string, nItems)

	for i := 0; i < nItems; i++ {
		key := fmt.Sprintf("test_flush_key_%d", i)
		value := fmt.Sprintf("v%d", i)
		items[i] = &Item{
			Key:        key,
			Value:      []byte(value),
			Flags:      0,
			Expiration: 0,
		}
		keys[i] = key
	}
	mc.SetMulti(items)

	// Make sure we have some data in the cache cluster
	itemsGot, err := mc.GetMulti(keys)
	if err != nil || len(itemsGot) != nItems {
		t.Error(err)
	}

	// Flush the cache cluster
	flushedHosts, err = mc.FlushAll()
	if err != nil || len(flushedHosts) != len(mc.servers) {
		t.Error(err)
	}

	// Check if the cache cluster is flushed
	itemsGot, err = mc.GetMulti(keys)
	if err != ErrCacheMiss || len(itemsGot) != 0 {
		t.Error(err)
	}

}
