#pragma once
#include <sys/uio.h>
#include <stdint.h>
#include <cstring>
#include <list>
#include <vector>


namespace douban {
namespace mc {
namespace io {


/**
 * iovec based BufferWriter
 * writev / sendmsg
 **/
class BufferWriter {
 public:
  BufferWriter();
  ~BufferWriter();
  void reset();
  void reserve(size_t n);
  void takeBuffer(const char* const buf, size_t buf_len);
  void takeNumber(int64_t val);
  const struct iovec* const getReadPtr(size_t &n);
  void commitRead(size_t nSent);
  void rewind();
  size_t msgIovlen();
  const bool isRead();

 protected:
  std::vector<struct iovec> m_iovec;
  std::vector<struct iovec> m_originalIovec;
  std::vector<char*>  m_unsignedStringList;

  // the index of iovec vector we'll read next
  size_t m_readIdx;

  // the number of iovec left to read
  size_t m_msgIovlen;
};

inline const bool BufferWriter::isRead() {
  return m_readIdx != 0;
}

} // namespace io
} // namespace mc
} // namespace douban
