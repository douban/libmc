package golibmc

import "fmt"
import "time"
import "strings"
import "testing"
import "strconv"

const LocalMC = "localhost:21211"
const ErrorSet = "Error on Set"
const ErrorGetAfterSet = "Error on Get after set"

func newSimpleClient(n int) *Client {
	servers := make([]string, n)
	for i := 0; i < n; i++ {
		servers[i] = fmt.Sprintf("localhost:%d", 21211+i)
	}
	noreply := false
	prefix := ""
	hashFunc := HashCRC32
	failover := false
	disableLock := false

	return New(servers, noreply, prefix, hashFunc, failover, disableLock)
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

func TestInputServer(t *testing.T) {
	servers := []string{"localhost:invalid_port"}
	noreply := false
	prefix := ""
	hashFunc := HashCRC32
	failover := false
	disableLock := false

	mc := New(servers, noreply, prefix, hashFunc, failover, disableLock)
	if mc != nil {
		t.Error()
	}
}

func TestStats(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testStats(mc, t)
	}
}

func testStats(mc *Client, t *testing.T) {
	statsDict, err := mc.Stats()
	if !(err == nil && len(statsDict) == len(mc.servers) && len(mc.servers) > 0) {
		t.Errorf("%d %d", len(statsDict), len(mc.servers))
	}

	if m, _ := statsDict[LocalMC]["version"]; len(m) == 0 {
		t.Error()
	}
}

func TestPrefix(t *testing.T) {
	c1 := Client{}
	testPrefix := "prefix"
	c2 := Client{prefix: testPrefix}
	key := "foo"

	if c1.addPrefix(key) != key {
		t.Error()
	}
	if c1.removePrefix(key) != key {
		t.Error()
	}

	key2 := strings.Join([]string{key, c2.prefix}, "")
	for _, key := range []string{key, key2} {

		prefixedKey := c2.addPrefix(key)
		if prefixedKey != strings.Join([]string{c2.prefix, key}, "") {
			t.Error()
		}
		if c2.removePrefix(prefixedKey) != key {
			t.Error()
		}
	}

	keyTmpls := []string{
		"%sforever/young",
		"forever%s/young",
		"forever/young/%s",
	}

	for _, keyTmpl := range keyTmpls {
		key := fmt.Sprintf(keyTmpl, c2.prefix)
		if key != c2.removePrefix(c2.addPrefix(key)) {
			t.Error()
		}
	}
}

func TestGetServerAddress(t *testing.T) {
	mc := newSimpleClient(1)
	if mc.GetServerAddressByKey("key") != LocalMC {
		t.Error()
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
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testSetNGet(mc, t)
	}
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
	if !(val == nil && err == nil) {
		t.Error(ErrorGetAfterSet)
	}

	dct, err = mc.GetMulti([]string{key})
	if !(len(dct) == 0 && err == nil) {
		t.Error(ErrorGetAfterSet)
	}
}

func TestSetMultiNGetMulti(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testSetMultiNGetMulti(mc, t)
	}
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

		if !strings.HasPrefix(string(itemGot.Value), "V") {
			t.Error()
		}
	}
	// Delete Values and Check Again
	mc.DeleteMulti(keys)
	itemsMap, err := mc.GetMulti(keys)
	if !(len(itemsMap) == 0 && err == nil) {
		t.Error()
	}
}

func TestIncrNDecr(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testIncrNDecr(mc, t)
	}
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
		t.Error()
	}

	if val, err := mc.Decr(key, 1); !(val == 99 && err == nil) {
		t.Error()
	}

	mc.Delete(key)

	if val, err := mc.Incr(key, 1); !(val == 0 && err != nil) {
		t.Error()
	}

	if val, err := mc.Decr(key, 1); !(val == 0 && err != nil) {
		t.Error()
	}
}

func TestLargeValue(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testLargeValue(mc, t)
	}
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
		t.Error()
	}
}

func TestCasAndGets(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testCasAndGets(mc, t)
	}
}

func testCasAndGets(mc *Client, t *testing.T) {
	key := "test_cas_and_gets"
	val := []byte("o")
	val2 := []byte("2")
	item := &Item{Key: key, Value: val}
	err := mc.Set(item)
	if err != nil {
		t.Error()
	}
	item2, err := mc.Gets(key)
	if item2 == nil || err != nil {
		t.Error()
	}
	(*item).Value = val2
	mc.Set(item)
	item3, err := mc.Gets(key)
	if item3 == nil || err != nil {
		t.Error()
	}
	if string((*item2).Value) != string(val) ||
		string((*item3).Value) != string(val2) {
		t.Error()
	}
	if (*item2).casid == (*item3).casid {
		t.Error()
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
		t.Error()
	}

	itemGot, err := mc.Gets(key)
	if err != nil || string(itemGot.Value) != value {
		t.Error()
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
		t.Error()
	}

	val3, err := mc.Incr(key, 10)
	if err != nil || val3 != 0 {
		t.Error(val3, err)
	}

	itemGot, err = mc.Gets(key)
	if err != nil || string(itemGot.Value) != "32" {
		t.Error()
	}

	val4, err := mc.Decr(key, 12)
	if err != nil || val4 != 0 {
		t.Error()
	}

	itemGot, err = mc.Gets(key)
	if err != nil || string(itemGot.Value) != "20" {
		t.Error()
	}

	err = mc.Delete(key)
	if err != nil {
		t.Error()
	}
}

func TestTouch(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testTouch(mc, t)
	}
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
		t.Error()
	}

	if err := mc.Touch(key, -1); err != nil {
		t.Error()
	}

	if itemGot, err := mc.Get(key); err != nil || itemGot != nil {
		t.Error()
	}

	// Touch
	mc.Set(item)
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != value {
		t.Error()
	}
	if err := mc.Touch(key, 1); err != nil {
		t.Error()
	}
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != value {
		t.Error()
	}
	time.Sleep(1000 * time.Millisecond)
	if itemGot, err := mc.Get(key); err != nil || itemGot != nil {
		t.Error()
	}
}

func TestAdd(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testAdd(mc, t)
	}
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
		t.Error()
	}
	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != valueToAdd {
		t.Error()
	}
	if err := mc.Add(item); err == nil {
		t.Error()
	}
	key2 := "test_add2"
	item.Key = key2
	mc.Delete(key2)
	if err := mc.Add(item); err != nil {
		t.Error()
	}
}

func TestReplace(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testReplace(mc, t)
	}
}

func testReplace(mc *Client, t *testing.T) {
	key := "test_replace"
	item := &Item{
		Key:   key,
		Value: []byte(""),
	}
	mc.Delete(key)

	if err := mc.Replace(item); err == nil {
		t.Error()
	}
	item.Value = []byte("b")
	if err := mc.Set(item); err != nil {
		t.Error()
	}
	item.Value = []byte("a")

	if err := mc.Replace(item); err != nil {
		t.Error()
	}

	if itemGot, err := mc.Get(key); err != nil || string(itemGot.Value) != "a" {
		t.Error()
	}
}

func TestPrepend(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testPrepend(mc, t)
	}
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
		t.Error()
	}

	item.Value = []byte(value2)
	if err := mc.Set(item); err != nil {
		t.Error()
	}

	item.Value = []byte(value)
	if err := mc.Prepend(item); err != nil {
		t.Error()
	}

	item.Value = []byte(value3)
	if err := mc.Prepend(item); err != nil {
		t.Error()
	}

	itemGot, err := mc.Get(key)
	if !(err == nil && string(itemGot.Value) == value3+value) {
		t.Error()
	}
}

func TestAppend(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testAppend(mc, t)
	}
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
		t.Error()
	}

	item.Value = []byte(value2)
	if err := mc.Set(item); err != nil {
		t.Error()
	}

	item.Value = []byte(value)
	if err := mc.Append(item); err != nil {
		t.Error()
	}

	item.Value = []byte(value3)
	if err := mc.Append(item); err != nil {
		t.Error()
	}

	itemGot, err := mc.Get(key)
	if !(err == nil && string(itemGot.Value) == value+value3) {
		_ = itemGot
		t.Errorf("[%s] : [%s]", string(itemGot.Value), value+value3)
	}
}

func TestSpecialItems(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testSpecialItems(mc, t)
	}
}

func testSpecialItems(mc *Client, t *testing.T) {
	key := "a\r\nb"
	if err := mc.Delete(key); err == nil {
		t.Error()
	}
	value := ""
	item := &Item{
		Key:   key,
		Value: []byte(value),
	}
	if err := mc.Set(item); err == nil {
		t.Error()
	}
}

func TestInjection(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
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
		t.Error()
	}

	item.Key = invalidKey
	if err := mc.Set(item); err == nil {
		t.Error()
	}

	item.Value = []byte("set injected 0 3600 10\r\n1234567890")
	if err := mc.Set(item); err == nil {
		t.Error()
	}

	it, err := mc.Get("injected")
	if it != nil || err != nil {
		t.Error()
	}

	item.Key = "key1"
	item.Value = []byte("1234567890")

	if err := mc.Set(item); err != nil {
		t.Error()
	}

	item.Key = "key1 0"
	item.Value = []byte("123456789012345678901234567890\r\nset injected 0 3600 3\r\nINJ\r\n")

	if err := mc.Set(item); err == nil {
		t.Error()
	}

	item, err = mc.Get("injected")
	if item != nil || err != nil {
		t.Error()
	}
}

func TestMaxiov(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		testMaxiov(mc, t)
	}
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
	if len(itemsGot) != 0 || err != nil {
		t.Error()
	}
}

func TestQuit(t *testing.T) {
	for _, nServers := range []int{1, 2, 10} {
		mc := newSimpleClient(nServers)
		mc2 := newSimpleClient(nServers)
		testQuit(mc, mc2, t)
	}
}

func testQuit(mc, mc2 *Client, t *testing.T) {
	key := "all_is_well"
	value := "yes"
	item := &Item{
		Key:        key,
		Value:      []byte(value),
		Flags:      0,
		Expiration: 0,
	}
	if err := mc.Delete(key); err != nil {
		t.Error()
	}
	if err := mc.Set(item); err != nil {
		t.Error()
	}
	if err := mc2.Delete(key); err != nil {
		t.Error()
	}
	if err := mc2.Set(item); err != nil {
		t.Error()
	}
	if versions, err := mc.Version(); err != nil || len(versions) == 0 {
		t.Error()
	}
	if versions, err := mc2.Version(); err != nil || len(versions) == 0 {
		t.Error()
	}

	st1, err1 := mc.Stats()
	st2, err2 := mc2.Stats()
	if err1 != nil || err2 != nil {
		t.Error()
	}

	nc1, err1 := strconv.Atoi(st1[LocalMC]["curr_connections"])
	nc2, err2 := strconv.Atoi(st2[LocalMC]["curr_connections"])
	if err1 != nil || err2 != nil {
		t.Error()
	}
	if nc1 != nc2 {
		t.Errorf("nc1: %d, nc2: %d", nc1, nc2)
	}

	mc.Quit()
	st2, err := mc2.Stats()
	nc2, err = strconv.Atoi(st2[LocalMC]["curr_connections"])

	if nc1-1 != nc2 {
		t.Errorf("nc1: %d, nc2: %d", nc1, nc2)
	}

	// back to life immediately
	itemGot, err := mc.Get(key)
	if err != nil || string(itemGot.Value) != value {
		t.Error()
	}
}

func BenchmarkSetAndGet(b *testing.B) {
	mc := newSimpleClient(1)
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
