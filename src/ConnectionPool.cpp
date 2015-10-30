#include <sys/socket.h>
#include <poll.h>
#include <errno.h>
#include <stdlib.h>
#include <list>
#include <vector>
#include <algorithm>

#include "ConnectionPool.h"
#include "Utility.h"
#include "Keywords.h"
#include "Parser.h"

using std::vector;

using douban::mc::keywords::kCRLF;
using douban::mc::keywords::kSPACE;
using douban::mc::keywords::k_NOREPLY;

using douban::mc::hashkit::KetamaSelector;

using douban::mc::types::RetrievalResult;

namespace douban {
namespace mc {

ConnectionPool::ConnectionPool()
  : m_nActiveConn(0), m_nInvalidKey(0), m_conns(NULL), m_nConns(0),
    m_pollTimeout(MC_DEFAULT_POLL_TIMEOUT) {
}


ConnectionPool::~ConnectionPool() {
  delete[] m_conns;
}


void ConnectionPool::setHashFunction(hash_function_options_t fn_opt) {
  switch (fn_opt) {
    case OPT_HASH_MD5:
      m_connSelector.setHashFunction(&douban::mc::hashkit::hash_md5);
      break;
    case OPT_HASH_FNV1_32:
      m_connSelector.setHashFunction(&douban::mc::hashkit::hash_fnv1_32);
      break;
    case OPT_HASH_FNV1A_32:
      m_connSelector.setHashFunction(&douban::mc::hashkit::hash_fnv1a_32);
      break;
    case OPT_HASH_CRC_32:
      m_connSelector.setHashFunction(&douban::mc::hashkit::hash_crc_32);
      break;
    default:
      NOT_REACHED();
      break;
  }
}


int ConnectionPool::init(const char* const * hosts, const uint32_t* ports, const size_t n,
                         const char* const * aliases) {
  delete[] m_conns;
  m_connSelector.reset();
  int rv = 0;
  m_nConns = n;
  m_conns = new Connection[m_nConns];
  for (size_t i = 0; i < m_nConns; i++) {
    rv += m_conns[i].init(hosts[i], ports[i], aliases == NULL ? NULL : aliases[i]);
  }
  m_connSelector.addServers(m_conns, m_nConns);
  return rv;
}


const char* ConnectionPool::getServerAddressByKey(const char* key, size_t keyLen) {
  bool check_alive = false;
  Connection* conn = m_connSelector.getConn(key, keyLen, check_alive);
  if (conn == NULL) {
    return NULL;
  }
  return conn->name();
}


void ConnectionPool::enableConsistentFailover() {
  m_connSelector.enableFailover();
}


void ConnectionPool::disableConsistentFailover() {
  m_connSelector.disableFailover();
}


void ConnectionPool::dispatchStorage(op_code_t op,
                                      const char* const* keys, const size_t* keyLens,
                                      const flags_t* flags, const exptime_t exptime,
                                      const cas_unique_t* cas_uniques, const bool noreply,
                                      const char* const* vals, const size_t* val_lens,
                                      size_t nItems) {

  size_t i = 0, idx = 0;

  for (; i < nItems; ++i) {
    if (!utility::isValidKey(keys[i], keyLens[i])) {
      m_nInvalidKey += 1;
      continue;
    }
    Connection* conn = m_connSelector.getConn(keys[i], keyLens[i]);
    if (conn == NULL) {
      continue;
    }
    switch (op) {
      case SET_OP:
        conn->takeBuffer(keywords::kSET_, 4);
        break;
      case ADD_OP:
        conn->takeBuffer(keywords::kADD_, 4);
        break;
      case REPLACE_OP:
        conn->takeBuffer(keywords::kREPLACE_, 8);
        break;
      case APPEND_OP:
        conn->takeBuffer(keywords::kAPPEND_, 7);
        break;
      case PREPEND_OP:
        conn->takeBuffer(keywords::kPREPEND_, 8);
        break;
      case CAS_OP:
        conn->takeBuffer(keywords::kCAS_, 4);
        break;
      default:
        NOT_REACHED();
        break;
    }

    conn->takeBuffer(keys[i], keyLens[i]);
    conn->takeBuffer(kSPACE, 1);
    conn->takeNumber(flags[i]);
    conn->takeBuffer(kSPACE, 1);
    conn->takeNumber(exptime);
    conn->takeBuffer(kSPACE, 1);
    conn->takeNumber(val_lens[i]);
    if (op == CAS_OP) {
      conn->takeBuffer(kSPACE, 1);
      conn->takeNumber(cas_uniques[i]);
    }
    if (noreply) {
      conn->takeBuffer(k_NOREPLY, 8);
    } else {
      conn->addRequestKey(keys[i], keyLens[i]);
    }
    ++conn->m_counter;
    conn->takeBuffer(kCRLF, 2);
    conn->takeBuffer(vals[i], val_lens[i]);
    conn->takeBuffer(kCRLF, 2);
  }

  for (idx = 0; idx < m_nConns; idx++) {
    Connection* conn = m_conns + idx;
    if (conn->m_counter > 0) {
      conn->setParserMode(MODE_COUNTING);
      m_nActiveConn += 1;
      m_activeConns.push_back(conn);
    }
    // for ignore noreply
    conn->m_counter = conn->requestKeyCount();
    if (conn->m_counter > 0) {
      conn->getMessageResults()->reserve(conn->m_counter);
    }
  }
}


void ConnectionPool::dispatchRetrieval(op_code_t op, const char* const* keys,
                                  const size_t* keyLens, size_t n_keys) {
  size_t i = 0, idx = 0;
  for (; i < n_keys; ++i) {
    const char* key = keys[i];
    const size_t len = keyLens[i];
    if (!utility::isValidKey(key, len)) {
      m_nInvalidKey += 1;
      continue;
    }
    Connection* conn = m_connSelector.getConn(key, len);
    if (conn == NULL) {
      continue;
    }
    // debug("hash %s => %d (%p)", key, idx % m_nConns, conn);
    if (++conn->m_counter == 1) {
      switch (op) {
        case GET_OP:
          conn->takeBuffer(keywords::kGET, 3);
          break;
        case GETS_OP:
          conn->takeBuffer(keywords::kGETS, 4);
          break;
        default:
          NOT_REACHED();
          break;
      }
    }
    conn->takeBuffer(kSPACE, 1);
    conn->takeBuffer(key, len);
  }
  for (idx = 0; idx < m_nConns; idx++) {
    Connection* conn = m_conns + idx;
    if (conn->m_counter > 0) {
      conn->takeBuffer(kCRLF, 2);
      conn->setParserMode(MODE_END_STATE);
      m_nActiveConn += 1;
      m_activeConns.push_back(conn);
      conn->getRetrievalResults()->reserve(conn->m_counter);
    }
  }
  // debug("after dispatchRetrieval: m_nActiveConn: %d", this->m_nActiveConn);
}


void ConnectionPool::dispatchDeletion(const char* const* keys, const size_t* keyLens,
                                     const bool noreply, size_t nItems) {

  size_t i = 0, idx = 0;
  for (; i < nItems; ++i) {
    if (!utility::isValidKey(keys[i], keyLens[i])) {
      m_nInvalidKey += 1;
      continue;
    }
    Connection* conn = m_connSelector.getConn(keys[i], keyLens[i]);
    if (conn == NULL) {
      continue;
    }

    conn->takeBuffer(keywords::kDELETE_, 7);
    conn->takeBuffer(keys[i], keyLens[i]);
    if (noreply) {
      conn->takeBuffer(k_NOREPLY, 8);
    } else {
      conn->addRequestKey(keys[i], keyLens[i]);
    }
    ++conn->m_counter;
    conn->takeBuffer(kCRLF, 2);
  }

  for (idx = 0; idx < m_nConns; idx++) {
    Connection* conn = m_conns + idx;
    if (conn->m_counter > 0) {
      conn->setParserMode(MODE_COUNTING);
      m_nActiveConn += 1;
      m_activeConns.push_back(conn);
    }
    // for ignore noreply
    conn->m_counter = conn->requestKeyCount();
    if (conn->m_counter > 0) {
      conn->getMessageResults()->reserve(conn->m_counter);
    }
  }
}


void ConnectionPool::dispatchTouch(
    const char* const* keys, const size_t* keyLens,
    const exptime_t exptime, const bool noreply, size_t nItems) {

  size_t i = 0, idx = 0;
  for (; i < nItems; ++i) {
    if (!utility::isValidKey(keys[i], keyLens[i])) {
      m_nInvalidKey += 1;
      continue;
    }
    Connection* conn = m_connSelector.getConn(keys[i], keyLens[i]);
    if (conn == NULL) {
      continue;
    }

    conn->takeBuffer(keywords::kTOUCH_, 6);
    conn->takeBuffer(keys[i], keyLens[i]);
    conn->takeBuffer(kSPACE, 1);
    conn->takeNumber(exptime);
    if (noreply) {
      conn->takeBuffer(k_NOREPLY, 8);
    } else {
      conn->addRequestKey(keys[i], keyLens[i]);
    }
    ++conn->m_counter;
    conn->takeBuffer(kCRLF, 2);
  }

  for (idx = 0; idx < m_nConns; idx++) {
    Connection* conn = m_conns + idx;
    if (conn->m_counter > 0) {
      conn->setParserMode(MODE_COUNTING);
      m_nActiveConn += 1;
      m_activeConns.push_back(conn);
    }
    // for ignore noreply
    conn->m_counter = conn->requestKeyCount();
    if (conn->m_counter > 0) {
      conn->getMessageResults()->reserve(conn->m_counter);
    }
  }
}


void ConnectionPool::dispatchIncrDecr(op_code_t op, const char* key, const size_t keyLen,
                                      const uint64_t delta, const bool noreply) {
  if (!utility::isValidKey(key, keyLen)) {
    m_nInvalidKey += 1;
    return;
  }
  Connection* conn = m_connSelector.getConn(key, keyLen);
  if (conn == NULL) {
    return;
  }
  switch (op) {
    case INCR_OP:
      conn->takeBuffer(keywords::kINCR_, 5);
      break;
    case DECR_OP:
      conn->takeBuffer(keywords::kDECR_, 5);
      break;
    default:
      NOT_REACHED();
      break;
  }
  conn->takeBuffer(key, keyLen);
  conn->takeBuffer(kSPACE, 1);
  conn->takeNumber(delta);
  if (noreply) {
    conn->takeBuffer(k_NOREPLY, 8);
  } else {
    conn->addRequestKey(key, keyLen);
  }
  ++conn->m_counter;
  conn->takeBuffer(kCRLF, 2);

  conn->setParserMode(MODE_COUNTING);
  m_nActiveConn += 1;
  m_activeConns.push_back(conn);

  // for ignore noreply
  // before the below line, conn->m_counter is a counter regarding how many packet to *send*
  conn->m_counter = conn->requestKeyCount();
  // after the upper line, conn->m_counter is a counter regarding how many packet to *recv*
}


void ConnectionPool::broadcastCommand(const char * const cmd, const size_t cmdLens) {
  for (size_t idx = 0; idx < m_nConns; ++idx) {
    Connection* conn = m_conns + idx;
    if (!conn->alive()) {
      if (!conn->tryReconnect()) {
        continue;
      }
    }
    conn->takeBuffer(cmd, cmdLens);
    ++conn->m_counter;
    conn->takeBuffer(kCRLF, 2);
    conn->setParserMode(MODE_END_STATE);
    m_nActiveConn += 1;
    m_activeConns.push_back(conn);
  }
}

err_code_t ConnectionPool::waitPoll() {
  if (m_nActiveConn == 0) {
    if (m_nInvalidKey > 0) {
      return RET_INVALID_KEY_ERR;
    } else {
      // hard server error
      return RET_MC_SERVER_ERR;
    }
  }
  nfds_t n_fds = m_nActiveConn;
  pollfd_t pollfds[n_fds];

  Connection* fd2conn[n_fds];

  pollfd_t* pollfd_ptr = NULL;
  nfds_t fd_idx = 0;

  for (std::vector<Connection*>::iterator it = m_activeConns.begin(); it != m_activeConns.end();
       ++it, ++fd_idx) {
    Connection* conn = *it;
    pollfd_ptr = &pollfds[fd_idx];
    pollfd_ptr->fd = conn->socketFd();
    pollfd_ptr->events = POLLOUT;
    fd2conn[fd_idx] = conn;
  }

  err_code_t ret_code = RET_OK;
  while (m_nActiveConn) {
    int rv = poll(pollfds, n_fds, m_pollTimeout);
    if (rv == -1) {
      markDeadAll(pollfds, keywords::kPOLL_ERROR);
      ret_code = RET_POLL_ERR;
      break;
    } else if (rv == 0) {
      log_warn("poll timeout. (m_nActiveConn: %d)", m_nActiveConn);
      // NOTE: MUST reset all active TCP connections after timeout.
      markDeadAll(pollfds, keywords::kPOLL_TIMEOUT);
      ret_code = RET_POLL_TIMEOUT_ERR;
      break;
    } else {
      err_code_t err;
      for (fd_idx = 0; fd_idx < n_fds; fd_idx++) {
        pollfd_ptr = &pollfds[fd_idx];
        Connection* conn = fd2conn[fd_idx];

        if (pollfd_ptr->revents & (POLLERR | POLLHUP | POLLNVAL)) {
          markDeadConn(conn, keywords::kCONN_POLL_ERROR, pollfd_ptr);
          ret_code = RET_CONN_POLL_ERR;
          m_nActiveConn -= 1;
          goto next_fd;
        }

        // send
        if (pollfd_ptr->revents & POLLOUT) {
          // POLLOUT send
          ssize_t nToSend = conn->send();
          if (nToSend == -1) {
            markDeadConn(conn, keywords::kSEND_ERROR, pollfd_ptr);
            ret_code = RET_SEND_ERR;
            m_nActiveConn -= 1;
            goto next_fd;
          } else {
            // start to recv if any data is sent
            pollfd_ptr->events |= POLLIN;

            if (nToSend == 0) {
              // debug("[%d] all sent", pollfd_ptr->fd);
              pollfd_ptr->events &= ~POLLOUT;
              if (conn->m_counter == 0) {
                // just send, no recv for noreply
                --this->m_nActiveConn;
              }
            }
          }
        }

        // recv
        if (pollfd_ptr->revents & POLLIN) {
          // POLLIN recv
          ssize_t nRecv = conn->recv();
          if (nRecv == -1 || nRecv == 0) {
            markDeadConn(conn, keywords::kRECV_ERROR, pollfd_ptr);
            ret_code = RET_RECV_ERR;
            m_nActiveConn -= 1;
            goto next_fd;
          }

          conn->process(err);
          switch (err) {
            case RET_OK:
              pollfd_ptr->events &= ~POLLIN;
              --m_nActiveConn;
              break;
            case RET_INCOMPLETE_BUFFER_ERR:
              break;
            case RET_PROGRAMMING_ERR:
              markDeadConn(conn, keywords::kPROGRAMMING_ERROR, pollfd_ptr);
              ret_code = RET_PROGRAMMING_ERR;
              m_nActiveConn -= 1;
              goto next_fd;
              break;
            case RET_MC_SERVER_ERR:
              // soft server error
              markDeadConn(conn, keywords::kSERVER_ERROR, pollfd_ptr);
              ret_code = RET_MC_SERVER_ERR;
              m_nActiveConn -= 1;
              goto next_fd;
              break;
            default:
              NOT_REACHED();
              break;
          }
        }

next_fd: {}
      } // end for
    }
  }
  return ret_code;
}


void ConnectionPool::collectRetrievalResult(std::vector<retrieval_result_t*>& results) {
  for (std::vector<Connection*>::iterator it = m_activeConns.begin();
       it != m_activeConns.end(); ++it) {
    types::RetrievalResultList* rst = (*it)->getRetrievalResults();

    for (types::RetrievalResultList::iterator it2 = rst->begin(); it2 != rst->end(); ++it2) {
      RetrievalResult& r1 = *it2;
      if (r1.bytesRemain > 0) {
        // This may be triggered on get_multi when data_block
        // of one retrieval result is not complete yet.
        continue;
      }
      results.push_back(r1.inner());
    }
  }
}


void ConnectionPool::collectMessageResult(std::vector<message_result_t*>& results) {
  for (std::vector<Connection*>::iterator it = m_activeConns.begin();
       it != m_activeConns.end(); ++it) {
    types::MessageResultList* rst = (*it)->getMessageResults();

    for (types::MessageResultList::iterator it2 = rst->begin(); it2 != rst->end(); ++it2) {
      results.push_back(&(*it2));
    }
  }
}


void ConnectionPool::collectBroadcastResult(std::vector<broadcast_result_t>& results) {
  results.resize(m_nConns);
  for (size_t i = 0; i < m_nConns; ++i) {
    Connection* conn = m_conns + i;
    broadcast_result_t* conn_result = &results[i];
    conn_result->host = const_cast<char*>(conn->name());
    types::LineResultList* rst = conn->getLineResults();
    conn_result->len = rst->size();

    if (conn_result->len == 0) {
      conn_result->lines = NULL;
      conn_result->line_lens = NULL;
      continue;
    }
    conn_result->lines = new char*[conn_result->len];
    conn_result->line_lens = new size_t[conn_result->len];

    int j = 0;
    for (types::LineResultList::iterator it2 = rst->begin(); it2 != rst->end(); ++it2, ++j) {
      types::LineResult* r1 = &(*it2);
      conn_result->lines[j] = r1->inner(conn_result->line_lens[j]);
    }
  }
}


void ConnectionPool::collectUnsignedResult(std::vector<unsigned_result_t*>& results) {
  if (m_activeConns.size() == 1) {
    types::UnsignedResultList* numericRst =  m_activeConns.front()->getUnsignedResults();
    types::MessageResultList* msgRst = m_activeConns.front()->getMessageResults();

    if (numericRst->size() == 1) {
      results.push_back(&numericRst->front());
    } else if (msgRst->size() == 1) {
      ASSERT(msgRst->front().type_ == MSG_NOT_FOUND);
      results.push_back(NULL);
    }
  }
}


void ConnectionPool::reset() {
  for (std::vector<Connection*>::iterator it = m_activeConns.begin();
       it != m_activeConns.end(); ++it) {
    (*it)->reset();
  }
  m_nActiveConn = 0;
  m_nInvalidKey = 0;
  m_activeConns.clear();
}


void ConnectionPool::setPollTimeout(int timeout) {
  m_pollTimeout = timeout;
}


void ConnectionPool::setConnectTimeout(int timeout) {
  for (size_t idx = 0; idx < m_nConns; ++idx) {
    Connection* conn = m_conns + idx;
    conn->setConnectTimeout(timeout);
  }
}


void ConnectionPool::setRetryTimeout(int timeout) {
  for (size_t idx = 0; idx < m_nConns; ++idx) {
    Connection* conn = m_conns + idx;
    conn->setRetryTimeout(timeout);
  }
}


void ConnectionPool::markDeadAll(pollfd_t* pollfds, const char* reason) {

  nfds_t fd_idx = 0;
  for (std::vector<Connection*>::iterator it = m_activeConns.begin();
      it != m_activeConns.end();
      ++it, ++fd_idx) {
    Connection* conn = *it;
    pollfd_t* pollfd_ptr = &pollfds[fd_idx];
    if (pollfd_ptr->events & (POLLOUT | POLLIN)) {
      conn->markDead(reason);
    }
  }
}


void ConnectionPool::markDeadConn(Connection* conn, const char* reason, pollfd_t* fd_ptr) {
  conn->markDead(reason);
  fd_ptr->events = ~POLLOUT & ~POLLIN;
  fd_ptr->fd = conn->socketFd();
}


} // namespace mc
} // namespace douban
