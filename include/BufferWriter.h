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
  size_t msgIovlen();

 protected:
  std::vector<struct iovec> m_iovec;
  std::vector<char*>  m_unsignedStringList;
  size_t m_readIdx;

  size_t m_msgIovlen;
};


} // namespace io
} // namespace mc
} // namespace douban
