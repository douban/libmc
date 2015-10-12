package libmc

import "fmt"
import "strings"
import "testing"

const LOCAL_MC = "localhost:21211"
const ERROR_GENERAL = "Error"
const ERROR_SET = "Error on Set"
const ERROR_GET_AS = "Error on Get after set"
const KEY_TO_SET = "google"
const VALUE_TO_SET = "goog"

var ERROR_VERSION = fmt.Sprintf("Bad version, make sure %s is started", LOCAL_MC)

func TestPrefix(t *testing.T) {
	c1 := Client{}
	testPrefix := "prefix"
	c2 := Client{prefix: testPrefix}
	key := "foo"

	if c1.addPrefix(key) != key {
		t.Error(ERROR_GENERAL)
	}
	if c1.removePrefix(key) != key {
		t.Error(ERROR_GENERAL)
	}

	key2 := strings.Join([]string{key, c2.prefix}, "")
	for _, key := range []string{key, key2} {

		prefixedKey := c2.addPrefix(key)
		if prefixedKey != strings.Join([]string{c2.prefix, key}, "") {
			t.Error(ERROR_GENERAL)
		}
		if c2.removePrefix(prefixedKey) != key {
			t.Error(ERROR_GENERAL)
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
			t.Error(ERROR_GENERAL)
		}
	}
}

func TestSetNGet(t *testing.T) {
	client := new(Client)
	servers := []string{LOCAL_MC}
	noreply := false
	prefix := ""
	failover := false
	client.Init(servers, noreply, prefix, "TODO", failover)
	version, err := client.Version()
	if len(version) == 0 || err != nil {
		t.Error(ERROR_VERSION)
	}

	item := Item{
		Key:        KEY_TO_SET,
		Value:      []byte(VALUE_TO_SET),
		Flags:      0,
		Expiration: 0,
	}
	err = client.Set(&item)
	if err != nil {
		t.Error(ERROR_SET)
	}

	val, err := client.Get(KEY_TO_SET)
	if v := string((*val).Value); v != VALUE_TO_SET || err != nil {
		t.Error(ERROR_GET_AS)
	}

	dct, err := client.GetMulti([]string{KEY_TO_SET})
	if err != nil {
		t.Error(ERROR_GET_AS)
	}

	if len(dct) != 1 {
		t.Error(ERROR_GET_AS)
	}

	if v := string((*dct[KEY_TO_SET]).Value); v != VALUE_TO_SET {
		t.Error(ERROR_GET_AS)
	}

	err = client.Delete(KEY_TO_SET)
	if err != nil {
		t.Error(ERROR_GET_AS)
	}

	val, err = client.Get(KEY_TO_SET)
	if !(val == nil && err == nil) {
		t.Error(ERROR_GET_AS)
	}

	dct, err = client.GetMulti([]string{KEY_TO_SET})
	if !(len(dct) == 0 && err == nil) {
		t.Error(ERROR_GET_AS)
	}

	client.Destroy()
}

func BenchmarkSetAndGet(b *testing.B) {
	client := new(Client)
	servers := []string{LOCAL_MC}
	noreply := false
	prefix := ""
	failover := false
	client.Init(servers, noreply, prefix, "TODO", failover)
	item := Item{
		Key:        KEY_TO_SET,
		Value:      []byte(VALUE_TO_SET),
		Flags:      0,
		Expiration: 0,
	}
	client.Set(&item)
	for i := 0; i < b.N; i++ {
		client.Get(KEY_TO_SET)
	}
	client.Destroy()
}
