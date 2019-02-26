// A standalone client using golibmc which aims to do ad-hoc trials.

package main

import (
	"fmt"
	golibmc "github.com/douban/libmc/src"
	"time"
)

func newClient() *golibmc.Client {
	noreply := false
	prefix := ""
	hashFunc := golibmc.HashMD5
	failover := false
	disableLock := false

	// The following is an arbitrary server:port which cannot reach,
	// so it will timeout when we connect
	SERVERS := []string{"1.1.1.1:1 mc-server-will-timeout"}

	// SERVERS := []string{"localhost:21211 mc-server-local"}

	CONNECT_TIMEOUT := 10
	// CONNECT_TIMEOUT := 1000
	POLL_TIMEOUT := 300
	RETRY_TIMEOUT := 5

	client := golibmc.New(SERVERS, noreply, prefix, hashFunc, failover, disableLock)
	client.ConfigTimeout(golibmc.ConnectTimeout, time.Duration(CONNECT_TIMEOUT)*time.Millisecond)
	client.ConfigTimeout(golibmc.PollTimeout, time.Duration(POLL_TIMEOUT)*time.Millisecond)
	client.ConfigTimeout(golibmc.RetryTimeout, time.Duration(RETRY_TIMEOUT)*time.Second)
	return client
}

func main() {
	client := newClient()

	key := "hello"
	val := "world"

	err := client.Set(&golibmc.Item{Key: key, Value: []byte(val)})
	if err != nil {
		fmt.Printf("ERROR: client.Set: %v\n", err)
	}

	item, err2 := client.Get(key)
	if err2 != nil {
		fmt.Printf("ERROR: client.Get: %v\n", err2)
	} else {
		fmt.Println(string(item.Value))
	}
}
