#pragma once

#include <vector>
#include "Export.h"
#include "BufferReader.h"
#ifdef MC_USE_SMALL_VECTOR
#include "llvm/SmallVector.h"
#endif

namespace douban {
namespace mc {
namespace types {


class RetrievalResult {
 public:
  RetrievalResult();
  RetrievalResult(const RetrievalResult& other);
  ~RetrievalResult();

  douban::mc::io::TokenData key; // 24B
  douban::mc::io::TokenData data_block; // 24B
  cas_unique_t cas_unique; // 8B
  uint32_t bytesRemain; // 4B. bytes remain to read, complete data if this is 0
  uint32_t bytes; // 4B
  flags_t flags; // 4B
  uint8_t key_len; // 1B
  retrieval_result_t* inner();
 protected:
  retrieval_result_t m_inner;
};


void delete_broadcast_result(broadcast_result_t* ptr);


class LineResult {
 public:
  LineResult();
  LineResult(const LineResult& other);
  ~LineResult();
  douban::mc::io::TokenData line;
  size_t line_len;
  char* inner(size_t& n);
 protected:
  char* m_inner;
};


#ifdef MC_USE_SMALL_VECTOR
typedef llvm::SmallVector<types::RetrievalResult, 100> RetrievalResultList;
#else
typedef std::vector<types::RetrievalResult> RetrievalResultList;
#endif
typedef std::vector<message_result_t> MessageResultList;
typedef std::vector<types::LineResult> LineResultList;
typedef std::vector<unsigned_result_t> UnsignedResultList;


} // namespace types
} // namespace mc
} // namespace douban
