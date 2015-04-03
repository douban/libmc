#include <inttypes.h>
#include <cstdio>
#include <vector>

#include "BufferWriter.h"
#include "Utility.h"

namespace douban{
namespace mc {
namespace io {

BufferWriter::BufferWriter() :m_readIdx(0), m_msgIovlen(0) {
}


BufferWriter::~BufferWriter() {
  reset();
}


void BufferWriter::reset() {
  m_iovec.clear();
  for (std::vector<char*>::const_iterator it = m_unsignedStringList.begin();
       it != m_unsignedStringList.end(); ++it) {
    delete[] *it;
  }
  m_unsignedStringList.clear();
  m_readIdx = 0;
  m_msgIovlen = 0;
}


void BufferWriter::reserve(size_t n) {
  m_iovec.reserve(n);
}


void BufferWriter::takeBuffer(const char* const buf, size_t buf_len) {
  struct iovec iov;
  iov.iov_base = const_cast<char*>(buf);
  iov.iov_len = buf_len;
  m_iovec.push_back(iov);
  m_msgIovlen += 1;
}


void BufferWriter::takeNumber(int64_t val) {
  m_unsignedStringList.push_back(new char[32]);
  char* buf = m_unsignedStringList.back();
  struct iovec iov;

  iov.iov_base = buf;
  iov.iov_len = douban::mc::utility::int64ToCharArray(val, buf);
  m_iovec.push_back(iov);
  m_msgIovlen += 1;
}


#ifdef __APPLE__
const struct iovec* const BufferWriter::getReadPtr(int &n) {
#else
const struct iovec* const BufferWriter::getReadPtr(size_t &n) {
#endif
  n = m_msgIovlen;
  if (n > 0) {
    return &m_iovec[m_readIdx];
  }
  return NULL;
}


void BufferWriter::commitRead(size_t nSent) {
  while (m_msgIovlen > 0 && nSent >= m_iovec[m_readIdx].iov_len) {
    nSent -= m_iovec[m_readIdx].iov_len;
    ++m_readIdx;
    --m_msgIovlen;
  }

  if (nSent > 0) {
    struct iovec* iovPtr = &m_iovec[m_readIdx];
    iovPtr->iov_base = static_cast<char*>(iovPtr->iov_base) + nSent;
    iovPtr->iov_len -= nSent;
  }
}


size_t BufferWriter::msgIovlen() {
  return m_msgIovlen;
}


} // namespace io
} // namespace mc
} // namespace douban
