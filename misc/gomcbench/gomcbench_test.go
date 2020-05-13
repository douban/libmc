package gomcbench

import (
	"fmt"
	"strings"
	"testing"

	"github.com/bradfitz/gomemcache/memcache"
	golibmc "github.com/douban/libmc/src"
)

const NSERVERS int = 10
const key1 string = "google"

var smallValue []byte = []byte("google")                   // 6B
var largeValue []byte = []byte(strings.Repeat("A", 10240)) // 10 KB

func getGolibmcClient(nServers int) *golibmc.Client {
	servers := make([]string, nServers)
	for i := 0; i < nServers; i++ {
		servers[i] = fmt.Sprintf("localhost:%d", 21211+i)
	}
	noreply := false
	prefix := ""
	hashFunc := golibmc.HashCRC32
	failover := false
	disableLock := false
	return golibmc.New(servers, noreply, prefix, hashFunc, failover, disableLock)
}

func getGomemcacheClient(nServers int) *memcache.Client {
	servers := make([]string, nServers)
	for i := 0; i < nServers; i++ {
		servers[i] = fmt.Sprintf("localhost:%d", 21211+i)
	}

	return memcache.New(servers...)
}

func benchmarkGolibmcGetSingleValue(nTimes int, value []byte) {
	mc := getGolibmcClient(NSERVERS)
	item := golibmc.Item{
		Key:   key1,
		Value: value,
	}
	mc.Set(&item)
	for i := 0; i < nTimes; i++ {
		mc.Get(key1)
	}
}

func benchmarkGomemcacheGetSingleValue(nTimes int, value []byte) {
	mc := getGomemcacheClient(NSERVERS)
	item := memcache.Item{
		Key:   key1,
		Value: value,
	}
	mc.Set(&item)
	for i := 0; i < nTimes; i++ {
		mc.Get(key1)
	}
}

func benchmarkGolibmcGetMultiValues(nTimes int, nItems int, value []byte) {
	mc := getGolibmcClient(NSERVERS)
	keys := make([]string, nItems)
	items := make([]*golibmc.Item, nItems)
	for i := 0; i < nItems; i++ {
		key := fmt.Sprintf("test_multi_key_%d", i)
		items[i] = &golibmc.Item{
			Key:   key,
			Value: value,
		}
		keys[i] = key
	}

	mc.SetMulti(items)
	for i := 0; i < nTimes; i++ {
		mc.GetMulti(keys)
	}
}

func benchmarkGomemcacheGetMultiValues(nTimes int, nItems int, value []byte) {
	mc := getGomemcacheClient(NSERVERS)
	keys := make([]string, nItems)
	for i := 0; i < nItems; i++ {
		key := fmt.Sprintf("test_multi_key_%d", i)
		item := &memcache.Item{
			Key:   key,
			Value: value,
		}
		keys[i] = key
		mc.Set(item)
	}

	for i := 0; i < nTimes; i++ {
		mc.GetMulti(keys)
	}
}

// get small value

func BenchmarkGolibmcGetSingleSmallValue(b *testing.B) {
	benchmarkGolibmcGetSingleValue(b.N, smallValue)
}

func BenchmarkGomemcacheGetSingleSmallValue(b *testing.B) {
	benchmarkGomemcacheGetSingleValue(b.N, smallValue)
}

// get large value

func BenchmarkGolibmcGetSingleLargeValue(b *testing.B) {
	benchmarkGolibmcGetSingleValue(b.N, largeValue)
}

func BenchmarkGomemcacheGetSingleLargeValue(b *testing.B) {
	benchmarkGomemcacheGetSingleValue(b.N, largeValue)
}

// get multi(10) small value

func BenchmarkGolibmcGet10SmallValues(b *testing.B) {
	benchmarkGolibmcGetMultiValues(b.N, 10, smallValue)
}

func BenchmarkGomemcacheGet10SmallValues(b *testing.B) {
	benchmarkGomemcacheGetMultiValues(b.N, 10, smallValue)
}

// get multi(100) small value

func BenchmarkGolibmcGet100SmallValues(b *testing.B) {
	benchmarkGolibmcGetMultiValues(b.N, 100, smallValue)
}

func BenchmarkGomemcacheGet100SmallValues(b *testing.B) {
	benchmarkGomemcacheGetMultiValues(b.N, 100, smallValue)
}

// get multi(1000) small value

func BenchmarkGolibmcGet1000SmallValues(b *testing.B) {
	benchmarkGolibmcGetMultiValues(b.N, 1000, smallValue)
}

func BenchmarkGomemcacheGet1000SmallValues(b *testing.B) {
	benchmarkGomemcacheGetMultiValues(b.N, 1000, smallValue)
}

// get multi(10) large value

func BenchmarkGolibmcGet10LargeValues(b *testing.B) {
	benchmarkGolibmcGetMultiValues(b.N, 10, largeValue)
}

func BenchmarkGomemcacheGet10LargeValues(b *testing.B) {
	benchmarkGomemcacheGetMultiValues(b.N, 10, largeValue)
}

// get multi(100) large value

func BenchmarkGolibmcGet100LargeValues(b *testing.B) {
	benchmarkGolibmcGetMultiValues(b.N, 100, largeValue)
}

func BenchmarkGomemcacheGet100LargeValues(b *testing.B) {
	benchmarkGomemcacheGetMultiValues(b.N, 100, largeValue)
}

// get multi(1000) large value

func BenchmarkGolibmcGet1000LargeValues(b *testing.B) {
	benchmarkGolibmcGetMultiValues(b.N, 1000, largeValue)
}

func BenchmarkGomemcacheGet1000LargeValues(b *testing.B) {
	benchmarkGomemcacheGetMultiValues(b.N, 1000, largeValue)
}
