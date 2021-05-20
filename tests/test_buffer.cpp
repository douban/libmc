#include "Common.h"
#include "Export.h"
#include "BufferReader.h"
#include <cstring>
#include "gtest/gtest.h"

using douban::mc::io::BufferReader;
using douban::mc::io::DataBlock;
using douban::mc::io::TokenData;

#define ASSERT_N_STREQ(S1, S2, N) do {ASSERT_TRUE(0 == std::strncmp((S1), (S2), (N)));} while (0)


#define TEST_SKIP_BYTES_NO_THROW(N) \
  do { \
    err_code_t e; \
    reader.skipBytes(e, (N)); \
    ASSERT_EQ(e, RET_OK); \
  } while(0)

#define TEST_READ_UNSIGNED_NO_THROW(N) \
  do { \
    err_code_t e; \
    reader.readUnsigned(e, (N)); \
    ASSERT_EQ(e, RET_OK); \
  } while(0)

TEST(test_buffer, min_capacity) {
  DataBlock::setMinCapacity(0x2B);
  ASSERT_EQ(DataBlock::minCapacity(), 0x2B);
}


TEST(test_buffer, peek_empty) {
  err_code_t err = RET_OK;
  BufferReader reader;
  reader.peek(err, 0);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  err = RET_OK;
  reader.peek(err, 1);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);
}

TEST(test_buffer, peek_one) {
  err_code_t err = RET_OK;

  BufferReader reader;
  DataBlock::setMinCapacity(0x2B);

  reader.write(CSTR("\x2B"), 1);
  ASSERT_EQ(reader.size(), 1);
  ASSERT_EQ(reader.readLeft(), 1);
  ASSERT_EQ(reader.nDataBlock(), 1);

  ASSERT_EQ(reader.peek(err, 0), '\x2B');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.peek(err, 0), '\x2B');
  ASSERT_EQ(err, RET_OK);

  reader.peek(err, 1);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  reader.peek(err, 2);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  reader.write(CSTR("-~"), 2);
  ASSERT_EQ(reader.peek(err, 1), '-');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.peek(err, 2), '~');
  ASSERT_EQ(err, RET_OK);
  reader.peek(err, 3);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);
  TEST_SKIP_BYTES_NO_THROW(3);
}

TEST(test_buffer, peek) {
  BufferReader reader;

  char str[] = "1024233";
  size_t len_str = strlen(str);
  DataBlock::setMinCapacity(len_str);
  reader.write(str, len_str);
  reader.write(CSTR(" "), 1);
  reader.write(str, len_str);
  EXPECT_EQ(reader.size(), len_str * 2 + 1);
  EXPECT_EQ(reader.capacity(), DataBlock::minCapacity() * 3);
  EXPECT_EQ(reader.readLeft(), len_str * 2 + 1);
  err_code_t err;
  ASSERT_EQ(reader.peek(err, 0), str[0]);
  ASSERT_EQ(err, RET_OK);
  for (size_t i = 0; i < len_str; i++) {
    ASSERT_EQ(reader.peek(err, i), str[i]);
    ASSERT_EQ(err, RET_OK);
  }
  ASSERT_EQ(reader.peek(err, len_str), ' ');
  ASSERT_EQ(err, RET_OK);

  for (size_t i = len_str + 1; i < 2 * len_str + 1; i++) {
    ASSERT_EQ(reader.peek(err, i), str[i - len_str - 1]);
    ASSERT_EQ(err, RET_OK);
  }
  reader.peek(err, 2 * len_str + 1);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);
  ASSERT_EQ(reader.nDataBlock(), 3);
  TEST_SKIP_BYTES_NO_THROW(2 * len_str + 1);
}

TEST(test_buffer, commit_read) {
  err_code_t err;
  DataBlock::setMinCapacity(5);
  BufferReader reader;
  reader.write(CSTR("0123456789ABCDEF"), 16);
  ASSERT_EQ(reader.readLeft(), 16);
  TEST_SKIP_BYTES_NO_THROW(3);
  ASSERT_EQ(reader.peek(err, 0), '3');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.readLeft(), 13);
  TEST_SKIP_BYTES_NO_THROW(3);
  ASSERT_EQ(reader.peek(err, 0), '6');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.readLeft(), 10);
  TEST_SKIP_BYTES_NO_THROW(7);
  ASSERT_EQ(reader.peek(err, 0), 'D');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.readLeft(), 3);
  TEST_SKIP_BYTES_NO_THROW(3);
  ASSERT_EQ(reader.readLeft(), 0);
  reader.peek(err, 0);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);
}


TEST(test_buffer, read_until_empty) {
  err_code_t err;
  BufferReader reader;

  TokenData td;
  reader.readUntil(err, ' ', td);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  reader.write(CSTR("b"), 1);
  reader.readUntil(err, ' ', td);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  TEST_SKIP_BYTES_NO_THROW(1);
}


TEST(test_buffer, read_until) {
  err_code_t err;
  char str[] = "foo b baz";
  size_t len_str = strlen(str);
  DataBlock::setMinCapacity(MIN_DATABLOCK_CAPACITY);

  BufferReader reader;
  DataBlock* dbPtr = NULL;
  reader.write(str, len_str);
  TokenData td, td2;
  ASSERT_EQ(reader.readUntil(err, ' ', td), 3);
  ASSERT_EQ(err, RET_OK);

  // bypass ' '
  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);
  TEST_SKIP_BYTES_NO_THROW(1);

  ASSERT_EQ(reader.readUntil(err, ' ', td2), 1);
  ASSERT_EQ(err, RET_OK);

  ASSERT_EQ(td.size(), 1);
  ASSERT_EQ(td.front().size, 3);
  ASSERT_EQ(td.front().offset, 0);
  dbPtr = &(*td.front().iterator);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "foo", 3);
  freeTokenData(td);

  ASSERT_EQ(td2.size(), 1);
  ASSERT_EQ(td2.front().size, 1);
  ASSERT_EQ(td2.front().offset, 4);
  dbPtr = &(*td2.front().iterator);
  ASSERT_N_STREQ((*dbPtr)[td2.front().offset], "b", 1);
  freeTokenData(td2);

  // bypass ' '
  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);
  TEST_SKIP_BYTES_NO_THROW(1);

  // should rollback read cursor on error
  td2.clear();
  reader.readUntil(err, ' ', td2);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);
  ASSERT_EQ(reader.peek(err, 0), 'b');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.peek(err, 1), 'a');
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(reader.peek(err, 2), 'z');
  ASSERT_EQ(err, RET_OK);
  TEST_SKIP_BYTES_NO_THROW(3);
}


TEST(test_buffer, read_until_across_block) {
  err_code_t err;
  DataBlock::setMinCapacity(5);

  // xi gu|a chi| guan|g le |aaaaa|aaaaa|aaa !
  char str_list[][6] = {"xi gu", "a chi", " guan", "g le ", "aaaaa", "aaaaa", "aaa !"};
  size_t i = 0;
  BufferReader reader;
  DataBlock* dbPtr = NULL;
  for (i = 0; i < 7; i++) {
    reader.write(str_list[i], 5);
  }
  size_t n_token = 7;
  TokenData* tokenDataPtr = new TokenData[n_token];
  TokenData* tdPtr = NULL;
  for (i = 0; i < n_token - 1; i++) {
    reader.readUntil(err, ' ', tokenDataPtr[i]);
    ASSERT_EQ(err, RET_OK);
    EXPECT_EQ(reader.peek(err, 0), ' ');
    ASSERT_EQ(err, RET_OK);
    TEST_SKIP_BYTES_NO_THROW(1);
  }

  reader.readUntil(err, ' ', *(tokenDataPtr + n_token - 1));
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  // xi
  tdPtr = tokenDataPtr;
  ASSERT_EQ(tdPtr->size(), 1);
  ASSERT_EQ(tdPtr->front().size, 2);
  ASSERT_EQ(tdPtr->front().offset, 0);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "xi", 2);

  // gu|a
  tdPtr = tokenDataPtr + 1;
  ASSERT_EQ(tdPtr->size(), 2);
  ASSERT_EQ(tdPtr->front().size, 2);
  ASSERT_EQ(tdPtr->front().offset, 3);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "gu", 2);
  // tdPtr->pop_front();
  tdPtr->erase(tdPtr->begin());
  ASSERT_EQ(tdPtr->front().size, 1);
  ASSERT_EQ(tdPtr->front().offset, 0);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "a", 1);

  // chi
  tdPtr = tokenDataPtr + 2;
  ASSERT_EQ(tdPtr->size(), 1);
  ASSERT_EQ(tdPtr->front().size, 3);
  ASSERT_EQ(tdPtr->front().offset, 2);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "chi", 3);

  // guan|g
  tdPtr = tokenDataPtr + 3;
  ASSERT_EQ(tdPtr->size(), 2);
  ASSERT_EQ(tdPtr->front().size, 4);
  ASSERT_EQ(tdPtr->front().offset, 1);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "guan", 4);
  // tdPtr->pop_front();
  tdPtr->erase(tdPtr->begin());
  ASSERT_EQ(tdPtr->front().size, 1);
  ASSERT_EQ(tdPtr->front().offset, 0);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "g", 1);

  // le
  tdPtr = tokenDataPtr + 4;
  ASSERT_EQ(tdPtr->size(), 1);
  ASSERT_EQ(tdPtr->front().size, 2);
  ASSERT_EQ(tdPtr->front().offset, 2);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "le", 2);

  // aaaaa|aaaaa|aaa
  tdPtr = tokenDataPtr + 5;
  ASSERT_EQ(tdPtr->size(), 3);
  ASSERT_EQ(tdPtr->front().size, 5);
  ASSERT_EQ(tdPtr->front().offset, 0);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "aaaaa", 5);
  // tdPtr->pop_front();
  tdPtr->erase(tdPtr->begin());
  ASSERT_EQ(tdPtr->front().size, 5);
  ASSERT_EQ(tdPtr->front().offset, 0);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "aaaaa", 5);
  // tdPtr->pop_front();
  tdPtr->erase(tdPtr->begin());
  ASSERT_EQ(tdPtr->front().size, 3);
  ASSERT_EQ(tdPtr->front().offset, 0);
  dbPtr = &*(tdPtr->front().iterator);
  dbPtr->release(tdPtr->front().size);
  ASSERT_N_STREQ((*dbPtr)[tdPtr->front().offset], "aaa", 3);

  ASSERT_EQ(reader.peek(err, 0), '!');
  ASSERT_EQ(err, RET_OK);
  TEST_SKIP_BYTES_NO_THROW(1);
  delete[] tokenDataPtr;
}

TEST(test_buffer, read_unsigned_empty) {
  err_code_t err;
  char str[] = "6";
  size_t len_str = strlen(str);
  uint64_t val;
  DataBlock::setMinCapacity(MIN_DATABLOCK_CAPACITY);
  BufferReader reader;
  reader.readUnsigned(err, val);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  reader.write(str, len_str);
  reader.readUnsigned(err, val);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  TEST_SKIP_BYTES_NO_THROW(len_str);
}


TEST(test_buffer, read_unsigned) {
  err_code_t err;
  char str[] = "1024 768\r798";
  size_t len_str = 12;
  DataBlock::setMinCapacity(MIN_DATABLOCK_CAPACITY);

  BufferReader reader;
  reader.write(str, len_str);
  uint64_t val;
  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 1024ULL);

  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);;
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 768ULL);

  EXPECT_EQ(reader.peek(err, 0), '\r');
  ASSERT_EQ(err, RET_OK);;
  TEST_SKIP_BYTES_NO_THROW(1);

  reader.readUnsigned(err, val);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  TEST_SKIP_BYTES_NO_THROW(3);
}


TEST(test_buffer, read_unsigned_across_block) {
  err_code_t err;
  DataBlock::setMinCapacity(5);
  BufferReader reader;
  reader.write(CSTR("12345"), 5);
  reader.write(CSTR("6 89 "), 5);
  reader.write(CSTR("423 0"), 5);
  reader.write(CSTR(" 10\r\n"), 5);

  uint64_t val;
  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 123456ULL);

  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);;
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 89ULL);

  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);;
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 423ULL);

  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);;
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 0ULL);

  EXPECT_EQ(reader.peek(err, 0), ' ');
  ASSERT_EQ(err, RET_OK);;
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 10ULL);

  reader.readUnsigned(err, val);
  ASSERT_EQ(err, RET_PROGRAMMING_ERR);
  TEST_SKIP_BYTES_NO_THROW(2);
}


TEST(test_buffer, read_bytes_empty) {
  err_code_t err;
  DataBlock::setMinCapacity(5);
  BufferReader reader;
  TokenData td;
  reader.readBytes(err, 0, td);
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(td.size(), 0);
}


TEST(test_buffer, read_bytes) {
  err_code_t err;
  DataBlock::setMinCapacity(5);
  BufferReader reader;
  TokenData td;
  DataBlock* dbPtr = NULL;
  reader.write(CSTR("12345"), 5);
  reader.write(CSTR("68964"), 5);
  reader.write(CSTR("\r\n"), 2);

  reader.readBytes(err, 6, td);
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(td.size(), 2);
  ASSERT_EQ(td.front().size, 5);
  ASSERT_EQ(td.front().offset, 0);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "12345", 5);
  // td.pop_front();
  td.erase(td.begin());

  ASSERT_EQ(td.front().size, 1);
  ASSERT_EQ(td.front().offset, 0);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "6", 1);

  td.clear();
  reader.readBytes(err, 4, td);
  ASSERT_EQ(err, RET_OK);

  ASSERT_EQ(td.size(), 1);
  ASSERT_EQ(td.front().size, 4);
  ASSERT_EQ(td.front().offset, 1);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "8964", 4);

  td.clear();
  reader.readBytes(err, 2, td);
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(td.size(), 1);
  ASSERT_EQ(td.front().size, 2);
  ASSERT_EQ(td.front().offset, 0);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "\r\n", 2);

  td.clear();
  reader.readBytes(err, 2, td);
  ASSERT_EQ(err, RET_INCOMPLETE_BUFFER_ERR);

  td.clear();
  reader.write(CSTR("syyin"), 5);
  reader.readBytes(err, 5, td);
  ASSERT_EQ(err, RET_OK);


  ASSERT_EQ(td.size(), 2);
  ASSERT_EQ(td.front().size, 3);
  ASSERT_EQ(td.front().offset, 2);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "syy", 3);
  // td.pop_front();
  td.erase(td.begin());
  ASSERT_EQ(td.front().size, 2);
  ASSERT_EQ(td.front().offset, 0);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "in", 2);
}


TEST(test_buffer, fuzzy) {
  err_code_t err;
  DataBlock::setMinCapacity(5);
  BufferReader reader;
  TokenData td;
  DataBlock* dbPtr = NULL;
  // VALUE foo 10 0 24\r\n
  // \r\n
  // END\r\n
  // []
  char str_list[][6] = {"VALUE", " foo ", "10 0 ", "24\r\n\r",
                        "\nEND\r", "\n"};
  for (int i = 0; i < 6; i++) {
    reader.write(str_list[i], strlen(str_list[i]));
  }
  uint64_t val, nBytes;
  reader.readUntil(err, ' ', td);
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(td.size(), 1);
  ASSERT_EQ(td.front().offset, 0);
  ASSERT_EQ(td.front().size, 5);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "VALUE", 5);
  TEST_SKIP_BYTES_NO_THROW(1);
  td.clear();

  reader.readUntil(err, ' ', td);
  ASSERT_EQ(err, RET_OK);
  ASSERT_EQ(td.size(), 1);
  ASSERT_EQ(td.front().offset, 1);
  ASSERT_EQ(td.front().size, 3);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  ASSERT_N_STREQ((*dbPtr)[td.front().offset], "foo", 3);
  TEST_SKIP_BYTES_NO_THROW(1);
  td.clear();

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 10);
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(nBytes);
  ASSERT_EQ(nBytes, 0);
  TEST_SKIP_BYTES_NO_THROW(1);

  TEST_READ_UNSIGNED_NO_THROW(val);
  ASSERT_EQ(val, 24);

  reader.readBytes(err, nBytes + 4, td);
  ASSERT_EQ(err, RET_OK);
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);
  // td.pop_front();
  td.erase(td.begin());
  dbPtr = &*(td.front().iterator);
  dbPtr->release(td.front().size);

  ASSERT_EQ(reader.peek(err, 0), 'E');
  ASSERT_EQ(err, RET_OK);
  TEST_SKIP_BYTES_NO_THROW(5);
}


TEST(test_buffer, scalability) {
  err_code_t err;
  DataBlock::setMinCapacity(3);
  BufferReader reader;
  reader.write(CSTR("012"), 3);
  reader.write(CSTR("345"), 3);
  reader.write(CSTR("678"), 3);
  ASSERT_EQ(reader.peek(err, 0), '0');
  ASSERT_EQ(err, RET_OK);

  TEST_SKIP_BYTES_NO_THROW(3);
  ASSERT_EQ(reader.peek(err, 0), '3');
  ASSERT_EQ(err, RET_OK);

  reader.write(CSTR("BCD"), 3);
  ASSERT_EQ(reader.capacity(), 12);
  TEST_SKIP_BYTES_NO_THROW(9);
  reader.reset();
  ASSERT_EQ(reader.capacity(), 3);
}
