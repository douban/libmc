#include <unistd.h>
#include <vector>

#include "Export.h"
#include "Common.h"
#include "Client.h"
#include "Keywords.h"

namespace douban {
namespace mc {

Client::Client() {
}


Client::~Client() {
}


void Client::config(config_options_t opt, int val) {
  switch (opt) {
    case CFG_POLL_TIMEOUT:
      setPollTimeout(val);
      break;
    case CFG_CONNECT_TIMEOUT:
      setConnectTimeout(val);
      break;
    case CFG_RETRY_TIMEOUT:
      setRetryTimeout(val);
      break;
    case CFG_HASH_FUNCTION:
      ConnectionPool::setHashFunction(static_cast<hash_function_options_t>(val));
    default:
      break;
  }
}


err_code_t Client::get(const char* const* keys, const size_t* keyLens, size_t nKeys,
                 retrieval_result_t*** results, size_t* nResults) {
  dispatchRetrieval(GET_OP, keys, keyLens, nKeys);
  err_code_t rv = waitPoll();
  collectRetrievalResult(results, nResults);
  return rv;
}


err_code_t Client::gets(const char* const* keys, const size_t* keyLens, size_t nKeys,
                 retrieval_result_t*** results, size_t* nResults) {
  dispatchRetrieval(GETS_OP, keys, keyLens, nKeys);
  err_code_t rv = waitPoll();
  collectRetrievalResult(results, nResults);
  return rv;
}


void Client::collectRetrievalResult(retrieval_result_t*** results, size_t* nResults) {
  assert(m_outRetrievalResultPtrs.size() == 0);
  ConnectionPool::collectRetrievalResult(m_outRetrievalResultPtrs);
  *nResults = m_outRetrievalResultPtrs.size();
  if (*nResults == 0) {
    *results = NULL;
  } else {
    *results = &m_outRetrievalResultPtrs.front();
  }
}


void Client::destroyRetrievalResult() {
  ConnectionPool::reset();
  m_outRetrievalResultPtrs.clear();
}


void Client::collectMessageResult(message_result_t*** results, size_t* nResults) {
  assert(m_outMessageResultPtrs.size() == 0);
  ConnectionPool::collectMessageResult(m_outMessageResultPtrs);
  *nResults = m_outMessageResultPtrs.size();

  if (*nResults == 0) {
    *results = NULL;
  } else {
    *results = &m_outMessageResultPtrs.front();
  }
}


void Client::destroyMessageResult() {
  ConnectionPool::reset();
  m_outMessageResultPtrs.clear();
}


#define IMPL_STORAGE_CMD(M, O) \
err_code_t Client::M(const char* const* keys, const size_t* key_lens, \
                 const flags_t* flags, const exptime_t exptime, \
                 const cas_unique_t* cas_uniques, const bool noreply, \
                 const char* const* vals, const size_t* val_lens, \
                 size_t nItems, message_result_t*** results, size_t* nResults) { \
  dispatchStorage((O), keys, key_lens, flags, exptime, cas_uniques, noreply, vals, \
                  val_lens, nItems); \
  err_code_t rv = waitPoll(); \
  collectMessageResult(results, nResults); \
  return rv;\
}

IMPL_STORAGE_CMD(set, SET_OP)
IMPL_STORAGE_CMD(add, ADD_OP)
IMPL_STORAGE_CMD(replace, REPLACE_OP)
IMPL_STORAGE_CMD(append, APPEND_OP)
IMPL_STORAGE_CMD(prepend, PREPEND_OP)
IMPL_STORAGE_CMD(cas, CAS_OP)
#undef IMPL_STORAGE_CMD

err_code_t Client::_delete(const char* const* keys, const size_t* key_lens,
                     const bool noreply, size_t nItems,
                     message_result_t*** results, size_t* nResults) {
  dispatchDeletion(keys, key_lens, noreply, nItems);
  err_code_t rv = waitPoll();
  collectMessageResult(results, nResults);
  return rv;
}


void Client::collectBroadcastResult(broadcast_result_t** results, size_t* nHosts) {
  assert(m_outBroadcastResultPtrs.size() == 0);
  *nHosts = m_nConns;
  ConnectionPool::collectBroadcastResult(m_outBroadcastResultPtrs);
  *results = &m_outBroadcastResultPtrs.front();
}


void Client::destroyBroadcastResult() {
  ConnectionPool::reset();
  for (std::vector<broadcast_result_t>::iterator it = m_outBroadcastResultPtrs.begin();
       it != m_outBroadcastResultPtrs.end(); ++it) {
    types::delete_broadcast_result(&(*it));
  }
  m_outBroadcastResultPtrs.clear();
}


err_code_t Client::version(broadcast_result_t** results, size_t* nHosts) {
  broadcastCommand(keywords::kVERSION, 7);
  err_code_t rv = waitPoll();
  collectBroadcastResult(results, nHosts);
  return rv;
}


err_code_t Client::quit() {
  broadcastCommand(keywords::kQUIT, 4);
  err_code_t rv = waitPoll();
  return rv;
}


err_code_t Client::stats(broadcast_result_t** results, size_t* nHosts) {
  broadcastCommand(keywords::kSTATS, 5);
  err_code_t rv = waitPoll();
  collectBroadcastResult(results, nHosts);
  return rv;
}


err_code_t Client::touch(const char* const* keys, const size_t* keyLens,
                   const exptime_t exptime, const bool noreply, size_t nItems,
                   message_result_t*** results, size_t* nResults) {
  dispatchTouch(keys, keyLens, exptime, noreply, nItems);
  err_code_t rv = waitPoll();
  collectMessageResult(results, nResults);
  return rv;
}


void Client::collectUnsignedResult(unsigned_result_t** results, size_t* nResults) {

  assert(m_outUnsignedResultPtrs.size() == 0);
  ConnectionPool::collectUnsignedResult(m_outUnsignedResultPtrs);
  *nResults = m_outUnsignedResultPtrs.size();

  if (*nResults == 0) {
    *results = NULL;
  } else {
    *results = m_outUnsignedResultPtrs.front();
  }
}

err_code_t Client::incr(const char* key, const size_t keyLen, const uint64_t delta,
                 const bool noreply,
                 unsigned_result_t** results, size_t* nResults) {
  dispatchIncrDecr(INCR_OP, key, keyLen, delta, noreply);
  err_code_t rv = waitPoll();
  collectUnsignedResult(results, nResults);
  return rv;
}


err_code_t Client::decr(const char* key, const size_t keyLen, const uint64_t delta,
                 const bool noreply,
                 unsigned_result_t** results, size_t* nResults) {
  dispatchIncrDecr(DECR_OP, key, keyLen, delta, noreply);
  err_code_t rv = waitPoll();
  collectUnsignedResult(results, nResults);
  return rv;
}


void Client::destroyUnsignedResult() {
  ConnectionPool::reset();
  m_outUnsignedResultPtrs.clear();
}


void Client::_sleep(uint32_t seconds) {
  usleep(seconds * 1000000);
}

} // namespace mc
} // namespace douban
