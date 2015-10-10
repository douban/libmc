package libmc

import "fmt"
import "testing"

const LOCAL_MC = "localhost:11211"
const ERROR_SET = "Error on Set"
const ERROR_GET_AS = "Error on Get after set"
const KEY_TO_SET = "google"
const VALUE_TO_SET = "goog"

var ERROR_VERSION = fmt.Sprintf("Bad version, make sure %s is started", LOCAL_MC)

func TestFoo(t *testing.T) {
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
	client.Destroy()
}
