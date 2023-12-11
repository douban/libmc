#pragma once

#include <vector>
#include "Client.h"

namespace douban {
namespace mc {

template <size_t N>
void duplicate_strings(const char* const * strs, const size_t n,
                       std::vector<std::array<char, N> >& out, std::vector<char*>& refs) {
    out.resize(n);
    refs.resize(n);
    for (int i = 0; i < n; i++) {
        if (strs == NULL || strs[i] == NULL) {
          out[i][0] = '\0';
          refs[i] = NULL;
          continue;
        }
        std::snprintf(out[i].data(), N, "%s", strs[i]);
        refs[i] = &out[i];
    }
}

class ClientPool {
public:
  ClientPool();
  ~ClientPool();
  void config(config_options_t opt, int val);
  int init(const char* const * hosts, const uint32_t* ports, const size_t n,
           const char* const * aliases = NULL);
  int updateServers(const char* const * hosts, const uint32_t* ports, const size_t n,
                    const char* const * aliases = NULL);
  Client* acquire();
  void release(Client* ref);

protected:
  int growPool(int by);

  bool m_opt_changed[CLIENT_CONFIG_OPTION_COUNT];
  int m_opt_value[CLIENT_CONFIG_OPTION_COUNT];
  std::vector<Client*> m_clients;
  int m_initial_clients;
  int m_max_clients;
  int m_max_growth;

  std::vector<std::array<char, MC_NI_MAXHOST>> m_hosts_data;
  std::vector<std::array<char, MC_NI_MAXHOST + 1 + MC_NI_MAXSERV>> m_aliases_data;
  std::vector<uint32_t> m_ports;

  std::vector<char*> m_hosts;
  std::vector<char*> m_aliases;
};

} // namespace mc
} // namespace douban