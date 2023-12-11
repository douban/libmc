#include <vector>
#include <atomic>
#include <execution>

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
    case CFG_INITIAL_CLIENTS:
      m_initial_clients = val;
      break;
    case CFG_MAX_CLIENTS:
      m_max_clients = val;
      break;
    case CFG_MAX_GROWTH:
      m_max_growth = val;
      break;
    default:
      if (opt < CLIENT_CONFIG_OPTION_COUNT) {
        m_opt_changed[opt] = true;
        m_opt_value[opt] = val;
        for (auto client : m_clients) {
          client.config(opt, val);
        }
      }
      break;
  }
}

int ClientPool::init(const char* const * hosts, const uint32_t* ports, const size_t n,
                     const char* const * aliases) {
  updateServers(hosts, ports, n, aliases);
  return growPool(m_initial_clients);
}

int ClientPool::updateServers(const char* const* hosts, const uint32_t* ports, const size_t n,
                              const char* const* aliases) {
  // TODO: acquire lock; one update at a time
  duplicate_strings(hosts, n, m_hosts_data, m_hosts);
  duplicate_strings(aliases, n, m_aliases_data, m_aliases);

  m_ports.resize(n);
  std::copy(ports, ports + n, m_ports.begin());

  std::atomic<int> rv = 0;
  std::for_each(std::execution::par, m_clients.begin(), m_clients.end(),
                [this, &rv](Client& c) {
    const int idx = &c - &m_clients[0];
    // TODO: acquire lock idx
    const int err = c.updateServers(m_hosts.data(), m_ports.data(),
                                    m_hosts.size(), m_aliases.data());
    if (err != 0) {
      rv.store(err, std::memory_order_relaxed);
    }
  });
  return rv;
}

int ClientPool::setup(Client* c) {
  for (int i = 0; i < CLIENT_CONFIG_OPTION_COUNT; i++) {
    if (m_opt_changed) {
      c->config(static_cast<config_options_t>(i), m_opt_value[i]);
    }
  }
  c->init(m_hosts.data(), m_ports.data(), m_hosts.size(), m_aliases.data());
}

int ClientPool::growPool(size_t by) {
  // TODO: acquire same lock as updateServers
  // TODO: client locks start busy
  assert(by > 0);
  size_t from = m_clients.size();
  m_clients.resize(from + by);
  auto start = m_clients.data() + from;
  std::for_each(std::execution::par, start, start + by, [this](Client& c) {
    setup(&c);
    const int idx = &c - &m_clients[0];
    // TODO: release lock idx
  });
}

Client* acquire() {
  // TODO
}

void release(Client* ref) {
  // TODO
}

} // namespace mc
} // namespace douban
