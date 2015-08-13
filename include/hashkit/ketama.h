#pragma once

#include <cassert>
#include <vector>
#include <algorithm>
#include "Connection.h"
#include "hashkit/hashkit.h"

namespace douban {
namespace mc {
namespace hashkit {


typedef struct  continuum_item_s {
  uint32_t hash_value;
  size_t conn_idx;
  douban::mc::Connection* conn;

  static struct compare_s {
    bool operator() (const struct continuum_item_s& left, const struct continuum_item_s& right) {
      return left.hash_value < right.hash_value;
    }
  } compare;
} continuum_item_t;


class KetamaSelector {
 public:
  KetamaSelector();
  void setHashFunction(hash_function_t fn);
  void enableFailover();
  void disableFailover();

  void reset();
  void addServers(douban::mc::Connection* conns, size_t nConns);

  int getServer(const char* key, size_t key_len, bool check_alive = true);
  douban::mc::Connection* getConn(const char* key, size_t key_len, bool check_alive = true);

 protected:
  std::vector<continuum_item_t>::iterator getServerIt(const char* key, size_t key_len,
                                                      bool check_alive);

  std::vector<continuum_item_t> m_continuum;
  size_t m_nServers;
  bool m_useFailover;
  hash_function_t m_hashFunction;
  static const size_t s_pointerPerHash;
  static const size_t s_pointerPerServer;
  static const hash_function_t s_defaultHashFunction;

#ifndef NDEBUG
  bool m_sorted;
#endif
};

} // namespace hashkit
} // namespace mc
} // namespace douban
