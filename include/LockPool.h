#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>
#include <deque>
#include <vector>
#include <atomic>
#include <memory>

namespace douban {
namespace mc {
  
class OrderedLock {
  std::queue<std::condition_variable*> m_fifo_locks;
  std::mutex m_fifo_access;
public:
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
  std::deque<std::mutex**> m_available;
protected:
  std::mutex m_available_lock;
  std::vector<std::mutex*> m_thread_workers;
  std::atomic<bool> m_waiting;
public:

  LockPool() : m_waiting(false) {}
  ~LockPool() {
    std::lock_guard<std::mutex> freeing(m_available_lock);
    for (auto worker : m_thread_workers) {
      std::lock_guard<std::mutex> freeing_worker(*worker);
      delete worker;
    }
  }

  void addWorkers(size_t n) {
    for (int i = n; i > 0; i--) {
      m_thread_workers.emplace_back(new std::mutex);
    }
    std::lock_guard<std::mutex> queueing_growth(m_available_lock);
    // static_cast needed for some versions of C++
    std::transform(
      m_thread_workers.end() - n, m_thread_workers.end(), std::back_inserter(m_available),
      static_cast<std::mutex**(*)(std::mutex*&)>(std::addressof<std::mutex*>));
    m_waiting = true;
    for (int i = n; i-- > 0; !unlock()) {}
  }

  std::mutex** acquireWorker() {
    if (!m_waiting) {
      lock();
    }
    std::lock_guard<std::mutex> acquiring(m_available_lock);
    const auto res = m_available.front();
    m_available.pop_front();
    m_waiting = m_available.empty();
    return res;
  }

  void releaseWorker(std::mutex** worker) {
    std::lock_guard<std::mutex> acquiring(m_available_lock);
    m_available.push_front(worker);
    m_waiting = unlock();
  }

  size_t workerIndex(std::mutex** worker) {
    return worker - m_thread_workers.data();
  }
};

} // namespace mc
} // namespace douban
