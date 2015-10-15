#pragma once

#include <stdint.h>
#include <cassert>
#include <cstring>
#include <list>
#include <vector>

#include "Export.h"
#include "Common.h"
#include "DataBlock.h"

#ifdef MC_USE_SMALL_VECTOR
#include "llvm/SmallVector.h"
#endif

namespace douban {
namespace mc {
namespace io {

typedef std::list<DataBlock> DataBlockList;
typedef DataBlockList::iterator DataBlockListIterator;


struct DataCursor_s {
  DataBlockListIterator iterator;
  size_t offset;

  bool operator==(const DataCursor_s& other) const {
    return (this->iterator == other.iterator && this->offset == other.offset);
  }

  bool operator!=(const DataCursor_s& other) const {
    return (this->iterator != other.iterator || this->offset != other.offset);
  }
};
typedef struct DataCursor_s DataCursor;


typedef struct {
  DataBlockListIterator iterator;
  size_t offset;
  size_t size;
} DataBlockSlice;

#ifdef MC_USE_SMALL_VECTOR
typedef llvm::SmallVector<DataBlockSlice, 1> TokenData;
#else
typedef std::vector<DataBlockSlice> TokenData;
#endif

void freeTokenData(TokenData& td);
char* parseTokenData(TokenData& td, size_t reserved);
void copyTokenData(const TokenData& src, TokenData& dst);


class BufferReader {
 public:
  BufferReader();
  ~BufferReader();
  void reset();

  size_t prepareWriteBlock(size_t len);

  char* getWritePtr();
  void commitWrite(size_t len);
  void write(char* ptr, size_t len, bool copying = true);

  size_t capacity();
  size_t size();
  size_t readLeft();
  size_t nDataBlock();
  size_t nBytesRef();

  const char peek(err_code_t& err, size_t offset) const;

  size_t readUntil(err_code_t& err, char value, TokenData& tokenData);
  size_t skipUntil(err_code_t& err, char value);
  void readUnsigned(err_code_t& err, uint64_t& value);
  void readBytes(err_code_t& err, size_t len, TokenData& tokenData);
  void expectBytes(err_code_t& err, const char* str, size_t str_size);
  void skipBytes(err_code_t& err, size_t str_size);
  void setNextPreferedDataBlockSize(size_t n);
  size_t getNextPreferedDataBlockSize();

 protected:
  const char charAtCursor(DataCursor& cur) const;

  DataBlockList m_dataBlockList;
  size_t m_capacity;
  size_t m_size;
  size_t m_readLeft;
  DataCursor m_blockReadCursor;
  DataBlockListIterator m_blockWriteIterator;
  size_t m_nextPreferedDataBlockSize;
};


inline const char BufferReader::charAtCursor(DataCursor& cur) const {
  // NOTE: make sure cur is valid
  /**
   * DataBlock& db = *cur.iterator;
   * char* chrPtr = db[cur.offset];
   * return *chrPtr;
   **/
  return *((*cur.iterator)[cur.offset]);
}


inline size_t BufferReader::readLeft() {
  return m_readLeft;
}

} // namespace io
} // namespace mc
} // namespace douban
