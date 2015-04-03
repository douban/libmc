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
#ifdef __APPLE__
  const struct iovec* const getReadPtr(int &n);
#else
  const struct iovec* const getReadPtr(size_t &n);
#endif
  void commitRead(size_t nSent);
  size_t msgIovlen();

 protected:
  std::vector<struct iovec> m_iovec;
  std::vector<char*>  m_unsignedStringList;
  size_t m_readIdx;

#ifdef __APPLE__
  int m_msgIovlen;
#else
  size_t m_msgIovlen;
#endif
};


} // namespace io
} // namespace mc
} // namespace douban
