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

#define tprintf(str, ...) printf("tid = %lu: " str, syscall(SYS_gettid), ##__VA_ARGS__)

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
      tprintf("locked %lu already waiting signal           %p\n", m_fifo_locks.size(), &signal);
      m_fifo_locks.emplace_back(&signal);
      signal.wait(acquire);
      signal.notify_all();
      tprintf("unlocked %lu left waiting signal            %p\n", m_fifo_locks.size(), &signal);
      assert(acquire.owns_lock());
    } else {
      m_locked = true;
      tprintf("passed through unlocked fifo\n");
    }
    return acquire;
  }

  void unlock(std::unique_lock<std::mutex>& acquire, int i = 0) {
    if (m_fifo_locks.empty()) {
      tprintf("fifo already empty\n");
      m_locked = false;
    } else {
      std::condition_variable* signal = m_fifo_locks.front();
      auto it = m_fifo_locks.begin();
      tprintf("unlocking with %d available %lu waiting lock %p\n", i, m_fifo_locks.size(), signal);
      signal->notify_one();
      signal->wait(acquire);
      m_fifo_locks.erase(it);
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
      tprintf("pushing available mux %lu\n", from + i);
      m_muxes.push_back(&muxes[i]);
    }
    // static_cast needed for some versions of C++
    std::transform(
      muxes, muxes + n, std::back_inserter(m_thread_workers),
      static_cast<std::mutex*(*)(std::mutex&)>(std::addressof<std::mutex>));
    tprintf("available size is now %lu\n", m_available.size());
    for (int i = n; i > 0; i--) {
      unlock(growing_pool, m_available.size());
    }
  }

  int acquireWorker() {
    tprintf("acquire called\n");
    auto fifo_lock = lock();
    const auto res = m_available.front();
    m_available.pop_front();
    if (!m_available.empty()) {
      unlock(fifo_lock, m_available.size());
    }
    tprintf("acquiring %lu; available size is now %lu\n", res, m_available.size());
    assert(m_available.size() <= m_thread_workers.size());
    return res;
  }

  void releaseWorker(int worker) {
    std::unique_lock<std::mutex> growing_pool(m_fifo_access);
    m_available.push_front(worker);
    tprintf("releasing %d; available size is now %lu\n", worker, m_available.size());
    unlock(growing_pool, m_available.size());
  }
};

} // namespace mc
} // namespace douban
