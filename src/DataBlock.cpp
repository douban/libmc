#include <algorithm>

#include "DataBlock.h"
#include "Common.h"

namespace douban {
namespace mc {
namespace io {

DataBlock::DataBlock()
    :m_data(NULL), m_capacity(0), m_size(0), m_nBytesRef(0)
{ }


DataBlock::DataBlock(const DataBlock& other) {
  if (other.m_data != NULL) {
    log_err("copy constructor of DataBlock should never be "
            "called after initialization.");
    return;
  }
  assert(other.m_data == NULL && other.m_capacity == 0 &&
         other.m_size == 0 && other.m_nBytesRef == 0);

  this->m_data = other.m_data;
  this->m_capacity = other.m_capacity;
  this->m_size = other.m_size;
  this->m_nBytesRef = other.m_nBytesRef;
}


size_t DataBlock::s_minCapacity(MIN_DATABLOCK_CAPACITY);


DataBlock::~DataBlock() {
  delete[] m_data;
}


void DataBlock::init(size_t len) {
  if (m_data != NULL) {
    log_err("DataBlock(%p)::init should only be called once", this);
    return;
  }
  this->m_data = new char[len];
  this->m_capacity = len;
  this->m_nBytesRef = 0;
  this->m_size = 0;
}


void DataBlock::setMinCapacity(size_t len) {
  log_warn("make sure this line is never called in production");
  s_minCapacity = len;
}


size_t DataBlock::minCapacity() {
  return s_minCapacity;
}


void DataBlock::reset() {
  this->m_nBytesRef = 0;
  this->m_size = 0;
}


size_t DataBlock::capacity() {
  return m_capacity;
}


size_t DataBlock::push(size_t len) {
  assert(m_size + len <= m_capacity);
  this->acquire(len);
  return m_size += len;
}


char* DataBlock::getWritePtr() {
  if (m_size == m_capacity) {
    return NULL;
  }
  return  m_data + m_size;
}


size_t DataBlock::getWriteLeft() {
  return m_capacity - m_size;
}


size_t DataBlock::find(char c, size_t since) {
  char *p = std::find(m_data + since, m_data + m_size, c);
  return p - m_data;
}


bool is_not_digit(int c) {
  return  !('0' <= c && c <= '9');
}


size_t DataBlock::findNotNumeric(size_t since) {
  char *p = std::find_if(m_data + since, m_data + m_size, is_not_digit);
  return p - m_data;
}


bool DataBlock::reusable() {
  return m_nBytesRef == 0;
}


size_t DataBlock::nBytesRef() {
  return m_nBytesRef;
}

} // namespace io
} // namespace mc
} // namespace douban
