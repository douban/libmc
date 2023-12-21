//#include <execution>
//#include <thread>
#include <array>
#include "ClientPool.h"

namespace douban {
namespace mc {

// default max of 4 clients to match memcached's default of 4 worker threads
ClientPool::ClientPool(): m_initial_clients(1), m_max_clients(4), m_max_growth(4) {
  memset(m_opt_changed, false, sizeof m_opt_changed);
  memset(m_opt_value, 0, sizeof m_opt_value);
}

ClientPool::~ClientPool() {
}

void ClientPool::config(config_options_t opt, int val) {
  std::lock_guard config_pool(m_pool_lock);
  if (opt < CLIENT_CONFIG_OPTION_COUNT) {
    m_opt_changed[opt] = true;
    m_opt_value[opt] = val;
    for (auto &client : m_clients) {
      client.c.config(opt, val);
    }
    return;
  }
  std::unique_lock initializing(m_acquiring_growth);
  switch (val) {
    case CFG_INITIAL_CLIENTS:
      m_initial_clients = val;
      if (m_clients.size() < m_initial_clients) {
        growPool(m_initial_clients);
      }
      break;
    case CFG_MAX_CLIENTS:
      m_max_clients = val;
      break;
    case CFG_MAX_GROWTH:
      m_max_growth = val;
      break;
    default:
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
  std::lock_guard<std::mutex> updating(m_fifo_access);
  //std::for_each(std::execution::par_unseq, irange(), irange(m_clients.size()),
  std::for_each(irange(), irange(m_clients.size()),
                [this, &rv](int i) {
    std::lock_guard<std::mutex> updating_worker(*m_thread_workers[i]);
    const int err = m_clients[i].c.updateServers(
      m_hosts.data(), m_ports.data(), m_hosts.size(), m_aliases.data());
    if (err != 0) {
      rv.store(err, std::memory_order_relaxed);
    }
  });
  return rv;
}

int ClientPool::setup(Client* c) {
  for (int i = 0; i < CLIENT_CONFIG_OPTION_COUNT; i++) {
    if (m_opt_changed[i]) {
      c->config(static_cast<config_options_t>(i), m_opt_value[i]);
    }
  }
  return c->init(m_hosts.data(), m_ports.data(), m_hosts.size(), m_aliases.data());
}

// if called outside acquire, needs to own m_acquiring_growth
int ClientPool::growPool(size_t by) {
  assert(by > 0);
  std::lock_guard growing_pool(m_pool_lock);
  size_t from = m_clients.size();
  m_clients.resize(from + by);
  std::atomic<int> rv = 0;
  //std::for_each(std::execution::par_unseq, irange(from), irange(from + by),
  std::for_each(irange(from), irange(from + by),
                [this, &rv](int i) {
    const int err = setup(&m_clients[i].c);
    m_clients[i].index = i;
    if (err != 0) {
      rv.store(err, std::memory_order_relaxed);
    }
  });
  // adds workers with non-zero return values
  // if changed, acquire should probably raise rather than hang
  addWorkers(by);
  return rv;
}

inline bool ClientPool::shouldGrowUnsafe() {
  return m_clients.size() < m_max_clients && m_locked;
}

int ClientPool::autoGrow() {
  std::unique_lock<std::shared_mutex> growing(m_acquiring_growth);
  if (shouldGrowUnsafe()) {
    return growPool(MIN(m_max_clients - m_clients.size(),
                    MIN(m_max_growth, m_clients.size())));
  }
  return 0;
}

IndexedClient* ClientPool::_acquire() {
  m_acquiring_growth.lock_shared();
  const auto growing = shouldGrowUnsafe();
  m_acquiring_growth.unlock_shared();
  if (growing) {
    //std::thread acquire_overflow(&ClientPool::autoGrow, this);
    //acquire_overflow.detach();
    autoGrow();
  }

  int idx = acquireWorker();
  m_thread_workers[idx]->lock();
  return &m_clients[idx];
}

void ClientPool::_release(const IndexedClient* idx) {
  std::mutex* const * mux = &m_thread_workers[idx->index];
  (**mux).unlock();
  releaseWorker(idx->index);
}

Client* ClientPool::acquire() {
  return &_acquire()->c;
}

void ClientPool::release(const Client* ref) {
  // C std 6.7.2.1-13
  auto idx = reinterpret_cast<const IndexedClient*>(ref);
  return _release(idx);
}

} // namespace mc
} // namespace douban
