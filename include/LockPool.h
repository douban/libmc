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

#include <cstdarg>
#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <assert.h>

namespace douban {
namespace mc {

// https://stackoverflow.com/a/14792685/3476782
class OrderedLock {
  std::list<std::condition_variable*> m_fifo_locks;
protected:
  std::mutex m_fifo_access;
  bool m_locked;

protected:
  OrderedLock() : m_locked(true) {};
  std::unique_lock<std::mutex> lock() {
    std::unique_lock<std::mutex> acquire(m_fifo_access);
    if (m_locked) {
      std::condition_variable signal;
      m_fifo_locks.emplace_back(&signal);
      signal.wait(acquire);
      m_fifo_locks.pop_front();
      assert(acquire.owns_lock());
    } else {
      m_locked = true;
    }
    return acquire;
  }

  void unlock() {
    if (m_fifo_locks.empty()) {
      m_locked = false;
    } else {
      m_fifo_locks.front()->notify_one();
    }
  }
};

class LockPool : public OrderedLock {
  std::deque<size_t> m_available;
  std::list<std::mutex*> m_muxes;
  std::list<std::mutex*> m_mux_mallocs;

protected:
  std::deque<std::mutex*> m_thread_workers;

  LockPool() {}
  ~LockPool() {
    std::lock_guard<std::mutex> freeing(m_fifo_access);
    for (auto worker : m_thread_workers) {
      std::lock_guard<std::mutex> freeing_worker(*worker);
    }
    for (auto mem : m_muxes) {
      mem->std::mutex::~mutex();
    }
    for (auto mem : m_mux_mallocs) {
      delete[] mem;
    }
  }

  void addWorkers(size_t n) {
    std::unique_lock<std::mutex> growing_pool(m_fifo_access);
    const auto from = m_thread_workers.size();
    const auto muxes = new std::mutex[n];
    m_mux_mallocs.push_back(muxes);
    for (size_t i = 0; i < n; i++) {
      m_available.push_back(from + i);
      m_muxes.push_back(&muxes[i]);
    }
    // static_cast needed for some versions of C++
    std::transform(
      muxes, muxes + n, std::back_inserter(m_thread_workers),
      static_cast<std::mutex*(*)(std::mutex&)>(std::addressof<std::mutex>));
    for (int i = n; i > 0; i--) {
      unlock();
    }
  }

  int acquireWorker() {
    auto fifo_lock = lock();
    const auto res = m_available.front();
    m_available.pop_front();
    if (!m_available.empty()) {
      unlock();
    }
    assert(m_available.size() <= m_thread_workers.size());
    return res;
  }

  void releaseWorker(int worker) {
    std::unique_lock<std::mutex> growing_pool(m_fifo_access);
    m_available.push_front(worker);
    unlock();
  }
};

} // namespace mc
} // namespace douban
