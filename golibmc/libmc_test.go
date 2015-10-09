package libmc

import "fmt"
import "testing"

const LOCAL_MC = "localhost:11211"

var ERROR_VERSION = fmt.Sprintf("bad version, make sure %s is started", LOCAL_MC)

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
	client.Destroy()
}
