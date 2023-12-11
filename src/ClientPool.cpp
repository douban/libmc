#include <vector>

#include "ClientPool.h"

namespace douban {
namespace mc {

// default max of 4 clients to match memcached's default of 4 worker threads
ClientPool::ClientPool(): m_initial_clients(1), m_max_clients(4), m_max_growth(4) {
  memset(m_opt_changed, false, sizeof m_opt_changed);
}


ClientPool::ClientPool() {
}


void ClientPool::config(config_options_t opt, int val) {
  switch (val) {
    case CFG_MAX_CLIENTS:
      m_max_clients = val;
      break;
    default:
      if (opt < CLIENT_CONFIG_OPTION_COUNT) {
        m_opt_changed[opt] = true;
        m_opt_value[opt] = val;
        for (auto client : m_clients) {
          client->config(opt, val);
        }
      }
      break;
  }
}


int ClientPool::init(const char* const * hosts, const uint32_t* ports, const size_t n,
                     const char* const * aliases) {
  updateServers(hosts, ports, n, aliases);
  growPool(m_initial_clients);
}


int ClientPool::updateServers(const char* const* hosts, const uint32_t* ports, const size_t n,
                              const char* const* aliases) {
  duplicate_strings(hosts, n, m_hosts_data, m_hosts);
  duplicate_strings(aliases, n, m_aliases_data, m_aliases);

  m_ports.resize(n);
  std::copy(ports, ports + n, m_ports.begin());

  for (auto client : m_clients) {
    client->updateServers(hosts, ports, n, aliases);
  }
}


int ClientPool::growPool(int by) {
  // TODO
}


Client* acquire() {
  // TODO
}


void release(Client* ref) {
  // TODO
}


} // namespace mc
} // namespace douban