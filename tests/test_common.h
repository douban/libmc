#include <cassert>
#include "Client.h"
#include <string>


namespace douban {
namespace mc {
namespace tests {

extern void gen_random(char *s, const int len);

void gen_random(char *s, const int len) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    int i = 0;
    for (i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    s[len] = 0;
}


mc::Client* newClient(int n) {
  assert(n <= 20);
  const char * hosts[] = {
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1",
    "127.0.0.1"
  };
  const uint32_t ports[] = {
    21211, 21212, 21213, 21214, 21215, 21216, 21217, 21218, 21219, 21220,
    21221, 21222, 21223, 21224, 21225, 21226, 21227, 21228, 21229, 21230
  };
  const char * aliases[] = {
    "alfa",
    "bravo",
    "charlie",
    "delta",
    "echo",
    NULL,
    "golf",
    "hotel",
    "india",
    "juliett",
    "kilo",
    "lima",
    "mike",
    "november",
    "oscar",
    "papa",
    "quebec",
    "romeo",
    "sierra",
    "tango"
  };
  mc::Client* client = new mc::Client();
  client->config(CFG_HASH_FUNCTION, OPT_HASH_MD5);
  client->init(hosts, ports, n, aliases);
  broadcast_result_t* results;
  size_t nHosts;
  int ret = client->version(&results, &nHosts);
  client->destroyBroadcastResult();
  if (ret != 0) {
    delete client;
    return NULL;
  }
  return client;
}


std::string get_resource_path(const char* basename) {
  std::string this_path(__FILE__);
  int pos =  this_path.rfind("/");
  return this_path.substr(0, pos) + "/resources/" + std::string(basename);
}


} // namespace tests
} // namespace mc
} // namespace douban
