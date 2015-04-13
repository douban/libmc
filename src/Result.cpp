#include "Result.h"
#include "Common.h"

namespace douban {
namespace mc {
namespace types {


RetrievalResult::RetrievalResult() {
  this->cas_unique = 0;
  this->bytes = 0;
  this->bytesRemain = this->bytes + 1;
  this->flags = 0;
  this->key_len = 0;
  m_inner.key = NULL;
  m_inner.data_block = NULL;
}

RetrievalResult::RetrievalResult(const RetrievalResult& other) {
  copyTokenData(other.key, this->key);
  copyTokenData(other.data_block, this->data_block);

  this->cas_unique = other.cas_unique;
  this->bytesRemain = other.bytesRemain;
  this->bytes = other.bytes;
  this->flags = other.flags;
  this->key_len = other.key_len;
  this->m_inner.key = NULL;
  this->m_inner.data_block = NULL;
}


RetrievalResult::~RetrievalResult() {
  if (key.size() > 1) { // copy happened
    delete[] m_inner.key;
  }
  if (data_block.size() > 1) {
    delete[] m_inner.data_block;
  }
  freeTokenData(key);
  freeTokenData(data_block);
}

retrieval_result_t* RetrievalResult::inner() {
  if (m_inner.key == NULL) {
    m_inner.key = parseTokenData(this->key, this->key_len);
  }
  if (m_inner.data_block == NULL) {
    m_inner.data_block = parseTokenData(this->data_block, this->bytes);
  }
  m_inner.cas_unique = this->cas_unique; // 8B
  m_inner.bytes = this->bytes; // 4B
  m_inner.flags = this->flags;  // 2B
  m_inner.key_len = this->key_len; // 1B
  return &m_inner;
}


LineResult::LineResult() {
  this->m_inner = NULL;
  this->line_len = 0;
}

LineResult::LineResult(const LineResult& other) {
  this->line_len = other.line_len;
  copyTokenData(other.line, this->line);
  this->m_inner = NULL;
}


LineResult::~LineResult() {
  if (this->line.size() > 1) {
    delete[] this->m_inner;
  }
  freeTokenData(this->line);
}

char* LineResult::inner(size_t& n) {
  if (this->m_inner == NULL) {
    this->m_inner = parseTokenData(this->line, this->line_len);
  }
  n = this->line_len - 1;  // NOTE: LineResult is always ends with '\r', which should be ignored
  return this->m_inner;
}


void delete_broadcast_result(broadcast_result_t* ptr) {
  if (ptr->lines) {
    delete[] ptr->lines;
    ptr->lines = NULL;
  }
  if (ptr->line_lens) {
    delete[] ptr->line_lens;
    ptr->line_lens = NULL;
  }
}


} // namespace types
} // namespace mc
} // namespace douban
