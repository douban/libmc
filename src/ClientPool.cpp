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
  std::lock_guard config_pool(m_pool_lock);
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

int ClientPool::init(const char* const * hosts, const uint32_t* ports,
                     const size_t n, const char* const * aliases) {
  updateServers(hosts, ports, n, aliases);
  std::unique_lock initializing(m_acquiring_growth);
  return growPool(m_initial_clients);
}

int ClientPool::updateServers(const char* const* hosts, const uint32_t* ports,
                              const size_t n, const char* const* aliases) {
  std::lock_guard updating_clients(m_pool_lock);
  duplicate_strings(hosts, n, m_hosts_data, m_hosts);
  duplicate_strings(aliases, n, m_aliases_data, m_aliases);

  m_ports.resize(n);
  std::copy(ports, ports + n, m_ports.begin());

  std::atomic<int> rv = 0;
  std::lock_guard<std::mutex> updating(m_available_lock);
  std::for_each(std::execution::par_unseq, m_clients.begin(), m_clients.end(),
                [this, &rv](Client& c) {
    const int idx = &c - m_clients.data();
    std::lock_guard<std::mutex> updating_worker(*m_thread_workers[idx]);
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

// if called outside acquire, needs to own m_acquiring_growth
int ClientPool::growPool(size_t by) {
  assert(by > 0);
  std::lock_guard growing_pool(m_pool_lock);
  size_t from = m_clients.size();
  m_clients.resize(from + by);
  const auto start = m_clients.data() + from;
  std::atomic<int> rv = 0;
  std::for_each(std::execution::par_unseq, start, start + by,
                [this, &rv](Client& c) {
    const int err = setup(&c);
    if (err != 0) {
      rv.store(err, std::memory_order_relaxed);
    }
  });
  addWorkers(by);
  return rv;
}

bool ClientPool::shouldGrowUnsafe() {
  return m_clients.size() < m_max_clients && !m_waiting;
}

int ClientPool::autoGrowUnsafe() {
  return growPool(MIN(m_max_clients - m_clients.size(),
                  MIN(m_max_growth, m_clients.size())));
}

Client* ClientPool::acquire() {
  m_acquiring_growth.lock_shared();
  const auto growing = shouldGrowUnsafe();
  m_acquiring_growth.unlock_shared();
  if (growing) {
    m_acquiring_growth.lock();
    if (shouldGrowUnsafe()) {
      autoGrowUnsafe();
    }
    m_acquiring_growth.unlock();
  }

  std::mutex** mux = acquireWorker();
  (**mux).lock();
  return &m_clients[workerIndex(mux)];
}

void ClientPool::release(Client* ref) {
  const int idx = ref - m_clients.data();
  std::mutex** mux = &m_thread_workers[idx];
  (**mux).unlock();
  releaseWorker(mux);
}

} // namespace mc
} // namespace douban
