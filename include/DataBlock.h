#pragma once
#include <cassert>
#include <cstdlib>
#include "Common.h"


namespace douban {
namespace mc {
namespace io {

class DataBlock {
 public:
  DataBlock();
  DataBlock(const DataBlock&);
  ~DataBlock();
  static void setMinCapacity(size_t len);
  static size_t minCapacity();

  void init(size_t len);
  void reset();
  inline char* operator[](size_t offset) {
    assert(offset < m_size);
    return m_data + offset;
  }

  inline char* at(size_t offset) {
    assert(offset < m_size);
    return m_data + offset;
  }

  inline const char* operator[](size_t offset) const {
    assert(offset < m_size);
    return m_data + offset;
  }

  void acquire(size_t len);
  void release(size_t len);
  size_t size();
  size_t capacity();
  bool reusable();
  size_t nBytesRef();
  size_t push(size_t len);
  char* getWritePtr();
  size_t getWriteLeft();
  size_t find(char c, size_t since = 0);
  size_t findNotNumeric(size_t since = 0);

 protected:
  char* m_data;
  size_t m_capacity;
  size_t m_size;
  size_t m_nBytesRef;
  static size_t s_minCapacity;  // FIXME: bad design
};


inline void DataBlock::acquire(size_t len) {
  this->m_nBytesRef += len;
}

inline void DataBlock::release(size_t len) {
  assert(this->m_nBytesRef >= len);
  this->m_nBytesRef -= len;
}


inline size_t DataBlock::size() {
  return m_size;
}

} // namespace io
} // namespace mc
} // namespace douban
