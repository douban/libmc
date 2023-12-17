#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <deque>
#include <vector>
#include <atomic>
#include <memory>
#include <list>
#include <algorithm>

namespace douban {
namespace mc {

class OrderedLock {
  std::queue<std::condition_variable*> m_fifo_locks;
  std::mutex m_fifo_access;

protected:
  OrderedLock() {};
  void lock() {
    std::unique_lock<std::mutex> acquire(m_fifo_access);
    std::condition_variable signal;
    m_fifo_locks.emplace(&signal);
    signal.wait(acquire);
  }

  bool unlock() {
    std::unique_lock<std::mutex> acquire(m_fifo_access);
    if (m_fifo_locks.empty()) {
      return true;
    }
    m_fifo_locks.front()->notify_one();
    m_fifo_locks.pop();
    return m_fifo_locks.empty();
  }
};

class LockPool : OrderedLock {
  std::deque<size_t> m_available;
  std::list<std::mutex* const*> m_mux_mallocs;

protected:
  std::mutex m_available_lock;
  std::deque<std::mutex*> m_thread_workers;
  std::atomic<bool> m_waiting;

  LockPool() : m_waiting(false) {}
  ~LockPool() {
    std::lock_guard<std::mutex> freeing(m_available_lock);
    for (auto worker : m_thread_workers) {
      std::lock_guard<std::mutex> freeing_worker(*worker);
    }
    for (auto mem : m_mux_mallocs) {
      delete[] mem;
    }
  }

  void addWorkers(size_t n) {
    std::lock_guard<std::mutex> queueing_growth(m_available_lock);
    const auto from = m_thread_workers.size();
    for (size_t i = from + n; i > from; i--) {
      m_available.push_back(i);
    }
    const auto muxes = new std::mutex[n];
    m_mux_mallocs.push_back(&muxes);
    // static_cast needed for some versions of C++
    std::transform(
      muxes, muxes + n, std::back_inserter(m_thread_workers),
      static_cast<std::mutex*(*)(std::mutex&)>(std::addressof<std::mutex>));
    m_waiting = true;
    for (int i = n; i-- > 0; !unlock()) {}
  }

  int acquireWorker() {
    if (!m_waiting) {
      lock();
    }
    std::lock_guard<std::mutex> acquiring(m_available_lock);
    const auto res = m_available.front();
    m_available.pop_front();
    m_waiting = m_available.empty();
    return res;
  }

  void releaseWorker(int worker) {
    std::lock_guard<std::mutex> acquiring(m_available_lock);
    m_available.push_front(worker);
    m_waiting = unlock();
  }
};

} // namespace mc
} // namespace douban
