#include "hashkit/ketama.h"
#include <vector>
#include <algorithm>
#include "Common.h"


using douban::mc::Connection;

namespace douban {
namespace mc {
namespace hashkit {


const size_t KetamaSelector::s_pointerPerHash = 1;
const size_t KetamaSelector::s_pointerPerServer = 100;
const hash_function_t KetamaSelector::s_defaultHashFunction = &hash_md5;

KetamaSelector::KetamaSelector()
  :m_nServers(0), m_useFailover(false), m_hashFunction(NULL)
#ifndef NDEBUG
  , m_sorted(false)
#endif
{
}

void KetamaSelector::setHashFunction(hash_function_t fn) {
  m_hashFunction = fn;
}

void KetamaSelector::enableFailover() {
  m_useFailover = true;
}

void KetamaSelector::disableFailover() {
  m_useFailover = false;
}

void KetamaSelector::reset() {
  m_continuum.clear();
  m_nServers = 0;
}

void KetamaSelector::addServers(Connection* conns, size_t nConns) {

  // from: libmemcached/libmemcached/hosts.cc +303
  char sort_host[MC_NI_MAXHOST + 1 + MC_NI_MAXSERV + 1 + MC_NI_MAXSERV]= "";
  for (size_t i = 0; i < nConns; i++) {
    Connection* conn = &conns[i];
    int sort_host_len = 0;
    for (size_t pointer_idx= 0; pointer_idx < s_pointerPerServer / s_pointerPerHash;
         pointer_idx++) {
      if (conn->hasAlias()) {
          sort_host_len = snprintf(sort_host, sizeof(sort_host), "%s-%zu",
                                   conn->name(), pointer_idx);
      } else {
        if (conn->port() != MC_DEFAULT_PORT) {
          sort_host_len = snprintf(sort_host, sizeof(sort_host), "%s:%u-%zu",
                                   conn->host(), conn->port(), pointer_idx);
        } else {
          sort_host_len = snprintf(sort_host, sizeof(sort_host), "%s-%zu",
                                   conn->host(), pointer_idx);
        }
      }
      continuum_item_t item;
      // Equivalent to `MEMCACHED_BEHAVIOR_KETAMA_HASH` behavior in libmemcached,
      // but here it always use hash_md5.
      item.hash_value = hash_md5(sort_host, sort_host_len);
      item.conn_idx = i;
      item.conn = conn;
      m_continuum.push_back(item);
    }
  }

  m_nServers = nConns;

  std::sort(m_continuum.begin(), m_continuum.end(), continuum_item_t::compare);
#ifndef NDEBUG
  m_sorted = true;
#endif
}


std::vector<continuum_item_t>::iterator KetamaSelector::getServerIt(const char* key, size_t key_len,
                                                                    bool check_alive) {
#ifndef NDEBUG
  if (!m_sorted) {
    return m_continuum.end();
  }
#endif
  std::vector<continuum_item_t>::iterator it = m_continuum.end();
  switch (m_nServers) {
    case 0:
      return m_continuum.end();
      break;
    case 1:
      it = m_continuum.begin();
      break;
    default:
      continuum_item_t target_item;

      if (m_hashFunction == NULL) {
        m_hashFunction = s_defaultHashFunction;
        log_warn("hash function is not specified, use hash_md5");
      }
      target_item.hash_value = m_hashFunction(key, key_len);
      target_item.conn_idx = 0;
      target_item.conn = NULL;
      it = std::lower_bound(m_continuum.begin(), m_continuum.end(), target_item,
                            continuum_item_t::compare);
      break;
  }

  if (it == m_continuum.end()) {
    it = m_continuum.begin();
  }
  Connection* origin_conn = it->conn;

  bool is_alive = true;
  if (check_alive && origin_conn != NULL) {
     is_alive = origin_conn->alive();
  }

  if (!is_alive) {
    if (m_useFailover) {
      size_t max_iter = m_continuum.size();
      do {
        ++it;
        if (it == m_continuum.end()) {
          it = m_continuum.begin();
        }
        if (it->conn != origin_conn && it->conn->tryReconnect()) {
          origin_conn = it->conn;
          break;
        }
      } while (--max_iter);
      if (max_iter == 0) {
        log_warn("no server is avaliable(alive) for key: \"%.*s\"", static_cast<int>(key_len), key);
        return m_continuum.end();
      }
    } else {
      if (!it->conn->tryReconnect()) {
        return m_continuum.end();
      }
    }
  }

  return it;
}


int KetamaSelector::getServer(const char* key, size_t key_len, bool check_alive) {
  std::vector<continuum_item_t>::iterator it = getServerIt(key, key_len, check_alive);
  if (it == m_continuum.end()) {
    return -1;
  }
  return static_cast<int>(it->conn_idx);
}

Connection* KetamaSelector::getConn(const char* key, size_t key_len, bool check_alive) {
  std::vector<continuum_item_t>::iterator it = getServerIt(key, key_len, check_alive);
  if (it == m_continuum.end()) {
    return NULL;
  }
  return it->conn;
}


} // namespace hashkit
} // namespace mc
} // namespace douban
