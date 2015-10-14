#pragma once

#include <vector>
#include "Export.h"
#include "Result.h"
#include "ConnectionPool.h"


namespace douban {
namespace mc {

class Client : public ConnectionPool {
 public:
  Client();
  ~Client();
  void config(config_options_t opt, int val);
  // retrieval commands
  void destroyRetrievalResult();

#define DECL_RETRIEVAL_CMD(M) \
  err_code_t M(const char* const* keys, const size_t* keyLens, size_t nKeys, \
           retrieval_result_t*** results, size_t* nResults);
DECL_RETRIEVAL_CMD(get)
DECL_RETRIEVAL_CMD(gets)
#undef DECL_RETRIEVAL_CMD

  // storage commands
  void destroyMessageResult();
#define DECL_STORAGE_CMD(M) \
  err_code_t M(const char* const* keys, const size_t* key_lens, \
           const flags_t* flags, const exptime_t exptime, \
           const cas_unique_t* cas_uniques, const bool noreply, \
           const char* const* vals, const size_t* val_lens, \
           size_t nItems, message_result_t*** results, size_t* nResults)

  DECL_STORAGE_CMD(set);
  DECL_STORAGE_CMD(add);
  DECL_STORAGE_CMD(replace);
  DECL_STORAGE_CMD(append);
  DECL_STORAGE_CMD(prepend);
  DECL_STORAGE_CMD(cas);
#undef DECL_STORAGE_CMD
  err_code_t _delete(const char* const* keys, const size_t* key_lens,
               const bool noreply, size_t nItems,
               message_result_t*** results, size_t* nResults);

  // broadcast commands
  void destroyBroadcastResult();

  err_code_t version(broadcast_result_t** results, size_t* nHosts);
  err_code_t quit();
  err_code_t stats(broadcast_result_t** results, size_t* nHosts);

  // touch
  err_code_t touch(const char* const* keys, const size_t* keyLens,
             const exptime_t exptime, const bool noreply, size_t nItems,
             message_result_t*** results, size_t* nResults);

  // incr / decr
  void destroyUnsignedResult();
  err_code_t incr(const char* key, const size_t keyLen, const uint64_t delta,
           const bool noreply,
           unsigned_result_t** result, size_t* nResults);
  err_code_t decr(const char* key, const size_t keyLen, const uint64_t delta,
           const bool noreply,
           unsigned_result_t** result, size_t* nResults);

  void _sleep(uint32_t seconds); // check GIL in Python

 protected:
  void collectRetrievalResult(retrieval_result_t*** results, size_t* nResults);
  void collectMessageResult(message_result_t*** results, size_t* nResults);
  void collectBroadcastResult(broadcast_result_t** results, size_t* nHosts);
  void collectUnsignedResult(unsigned_result_t** results, size_t* nResults);

  std::vector<retrieval_result_t*> m_outRetrievalResultPtrs;
  std::vector<message_result_t*> m_outMessageResultPtrs;
  std::vector<broadcast_result_t> m_outBroadcastResultPtrs;
  std::vector<unsigned_result_t*> m_outUnsignedResultPtrs;
};

} // namespace mc
} // namespace douban
