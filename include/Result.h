#pragma once

#include <vector>
#include "BufferReader.h"
#ifdef MC_USE_SMALL_VECTOR
#include "llvm/SmallVector.h"
#endif

namespace douban {
namespace mc {
namespace types {


typedef int64_t exptime_t;
typedef uint32_t flags_t;
typedef uint64_t cas_unique_t;


// Retrieval Result

typedef struct {
  char* key; // 8B
  char* data_block; // 8B
  cas_unique_t cas_unique; // 8B
  uint32_t bytes; // 4B
  flags_t flags;  // 4B
  uint8_t key_len; // 1B
} retrieval_result_t;


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


// Message Result

enum message_result_type {
  MSG_EXISTS,
  MSG_OK,
  MSG_STORED,
  MSG_NOT_STORED,
  MSG_NOT_FOUND,
  MSG_DELETED,
  MSG_TOUCHED,
};


typedef struct {
  enum message_result_type type;
  char* key;
  size_t key_len;
} message_result_t;


// Broadcast Line Result

typedef struct {
  char* host;
  char** lines;
  size_t* line_lens;
  size_t len;
} broadcast_result_t;
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


// Unsigned (Integer) Result

typedef struct {
  char* key;
  size_t key_len;
  uint64_t value;
} unsigned_result_t;


#ifdef MC_USE_SMALL_VECTOR
typedef llvm::SmallVector<types::RetrievalResult, 100> RetrievalResultList;
#else
typedef std::vector<types::RetrievalResult> RetrievalResultList;
#endif
typedef std::vector<types::message_result_t> MessageResultList;
typedef std::vector<types::LineResult> LineResultList;
typedef std::vector<types::unsigned_result_t> UnsignedResultList;


} // namespace types
} // namespace mc
} // namespace douban
