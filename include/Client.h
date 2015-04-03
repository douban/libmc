#pragma once

#include <vector>

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
  int get(const char* const* keys, const size_t* keyLens, size_t nKeys,
           types::retrieval_result_t*** results, size_t* nResults);
  int gets(const char* const* keys, const size_t* keyLens, size_t nKeys,
           types::retrieval_result_t*** results, size_t* nResults);

  // storage commands
  void destroyMessageResult();
#define DECL_STORAGE_CMD(M) \
  int M(const char* const* keys, const size_t* key_lens, \
           const types::flags_t* flags, const types::exptime_t exptime, \
           const types::cas_unique_t* cas_uniques, const bool noreply, \
           const char* const* vals, const size_t* val_lens, \
           size_t nItems, types::message_result_t*** results, size_t* nResults)

  DECL_STORAGE_CMD(set);
  DECL_STORAGE_CMD(add);
  DECL_STORAGE_CMD(replace);
  DECL_STORAGE_CMD(append);
  DECL_STORAGE_CMD(prepend);
  DECL_STORAGE_CMD(cas);

  int _delete(const char* const* keys, const size_t* key_lens,
               const bool noreply, size_t nItems,
               types::message_result_t*** results, size_t* nResults);

  // broadcast commands
  void destroyBroadcastResult();

  int version(types::broadcast_result_t** results, size_t* nHosts);
  int stats(types::broadcast_result_t** results, size_t* nHosts);

  // touch
  int touch(const char* const* keys, const size_t* keyLens,
             const types::exptime_t exptime, const bool noreply, size_t nItems,
             types::message_result_t*** results, size_t* nResults);

  // incr / decr
  void destroyUnsignedResult();
  int incr(const char* key, const size_t keyLen, const uint64_t delta,
           const bool noreply,
           types::unsigned_result_t*** result, size_t* nResults);
  int decr(const char* key, const size_t keyLen, const uint64_t delta,
           const bool noreply,
           types::unsigned_result_t*** result, size_t* nResults);

  void _sleep(uint32_t seconds); // check GIL in Python

 protected:
  void collectRetrievalResult(types::retrieval_result_t*** results, size_t* nResults);
  void collectMessageResult(types::message_result_t*** results, size_t* nResults);
  void collectBroadcastResult(types::broadcast_result_t** results, size_t* nHosts);
  void collectUnsignedResult(types::unsigned_result_t*** results, size_t* nResults);

  std::vector<types::retrieval_result_t*> m_outRetrievalResultPtrs;
  std::vector<types::message_result_t*> m_outMessageResultPtrs;
  std::vector<types::broadcast_result_t> m_outBroadcastResultPtrs;
  std::vector<types::unsigned_result_t*> m_outUnsignedResultPtrs;
};

} // namespace mc
} // namespace douban
