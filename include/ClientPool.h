#pragma once

#include <shared_mutex>
#include <iterator>
#include <array>

#include "Client.h"
#include "LockPool.h"

namespace douban {
namespace mc {

template <size_t N>
void duplicate_strings(const char* const * strs, const size_t n,
                       std::deque<std::array<char, N> >& out, std::vector<char*>& refs) {
    out.resize(n);
    refs.resize(n);
    for (size_t i = 0; i < n; i++) {
        if (strs == NULL || strs[i] == NULL) {
          out[i][0] = '\0';
          refs[i] = NULL;
          continue;
        }
        std::snprintf(out[i].data(), N, "%s", strs[i]);
        refs[i] = out[i].data();
    }
}

class irange {
  int i;

public:
  using value_type = int;
  using pointer = const int*;
  using reference = const int&;
  using difference_type = int;
  using iterator_category = std::random_access_iterator_tag;

  explicit irange(int i) : i(i) {}

  reference operator*() const { return i; }
  pointer operator->() const { return &i; }
  value_type operator[](int n) const { return i + n; }
  friend bool operator< (const irange& lhs, const irange& rhs) { return lhs.i < rhs.i; }
  friend bool operator> (const irange& lhs, const irange& rhs) { return rhs < lhs; }
  friend bool operator<=(const irange& lhs, const irange& rhs) { return !(lhs > rhs); }
  friend bool operator>=(const irange& lhs, const irange& rhs) { return !(lhs < rhs); }
  friend bool operator==(const irange& lhs, const irange& rhs) { return lhs.i == rhs.i; }
  friend bool operator!=(const irange& lhs, const irange& rhs) { return !(lhs == rhs); }
  irange& operator++() { ++i; return *this; }
  irange& operator--() { --i; return *this; }
  irange operator++(int) { irange tmp = *this; ++tmp; return tmp; }
  irange operator--(int) { irange tmp = *this; --tmp; return tmp; }
  irange& operator+=(difference_type n) { i += n; return *this; }
  irange& operator-=(difference_type n) { i -= n; return *this; }
  friend irange operator+(const irange& lhs, difference_type n) { irange tmp = lhs; tmp += n; return tmp; }
  friend irange operator+(difference_type n, const irange& rhs) { return rhs + n; }
  friend irange operator-(const irange& lhs, difference_type n) { irange tmp = lhs; tmp -= n; return tmp; }
  friend difference_type operator-(const irange& lhs, const irange& rhs) { return lhs.i - rhs.i; }
};

typedef struct {
  Client c;
  int index;
} IndexedClient;

class ClientPool : LockPool {
public:
  ClientPool();
  ~ClientPool();
  void config(config_options_t opt, int val);
  int init(const char* const * hosts, const uint32_t* ports,
           const size_t n, const char* const * aliases = NULL);
  int updateServers(const char* const * hosts, const uint32_t* ports,
                    const size_t n, const char* const * aliases = NULL);
  IndexedClient* _acquire();
  void _release(const IndexedClient* idx);
  Client* acquire();
  void release(const Client* ref);

private:
  int growPool(size_t by);
  int setup(Client* c);
  inline bool shouldGrowUnsafe();
  int autoGrow();

  bool m_opt_changed[CLIENT_CONFIG_OPTION_COUNT];
  int m_opt_value[CLIENT_CONFIG_OPTION_COUNT];
  std::deque<IndexedClient> m_clients;
  size_t m_initial_clients;
  size_t m_max_clients;
  size_t m_max_growth;

  std::deque<std::array<char, MC_NI_MAXHOST> > m_hosts_data;
  std::deque<std::array<char, MC_NI_MAXHOST + 1 + MC_NI_MAXSERV> > m_aliases_data;
  std::vector<uint32_t> m_ports;

  std::vector<char*> m_hosts;
  std::vector<char*> m_aliases;

  std::mutex m_pool_lock;
  mutable std::shared_mutex m_acquiring_growth;
};

} // namespace mc
} // namespace douban
