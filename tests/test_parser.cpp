#include "Common.h"
#include "Result.h"
#include "BufferReader.h"
#include "Parser.h"
#include <cstring>
#include "gtest/gtest.h"

using douban::mc::types::RetrievalResult;

using douban::mc::io::BufferReader;
using douban::mc::io::DataBlock;
using douban::mc::io::TokenData;
using douban::mc::PacketParser;



TEST(test_parser, empty_result) {
  err_code_t err;
  BufferReader reader;
  PacketParser parser;
  parser.setMode(douban::mc::MODE_END_STATE);
  parser.setBufferReader(&reader);
  int i = 0;
  char input_buffer[][5] = {
    "E", "ND\r\n"
  };
  while (true) {
    if (i < 2) {
      reader.write(input_buffer[i], strlen(input_buffer[i]));
      ++i;
    }
    parser.process_packets(err);
    if (err == RET_INCOMPLETE_BUFFER_ERR) {
      continue;
    }
    break;
  }
  ASSERT_EQ(parser.getRetrievalResults()->size(), 0);
}


TEST(test_parser, multi_results) {
  err_code_t err;
  DataBlock::setMinCapacity(10);
  BufferReader reader;
  PacketParser parser;
  parser.setMode(douban::mc::MODE_END_STATE);
  parser.setBufferReader(&reader);
  size_t i = 0;

  char input_buffer[][100] = {
      "VALUE fo", "o 0 ", "14", "\r", "\n", "VALUE foo ", "0 14", "\r\n",
      "VALUE baz", " 1 ", "4 1024", "\r", "\n", "\r\n \"", "\r\n",
      "VALUE ba", "r 0 ", "4", "\r", "\n", "1234", "\r\n",
     "E", "ND\r\n"
  };
  size_t n_input = 24;

  while (true) {
    if (i < n_input) {
      ASSERT_NO_THROW(reader.write(input_buffer[i], strlen(input_buffer[i])));
      ++i;
    }
    parser.process_packets(err);
    if (err == RET_INCOMPLETE_BUFFER_ERR) {
      continue;
    }
    break;
  }
  RetrievalResult* res = NULL;
  retrieval_result_t* innerRes;
  ASSERT_EQ(parser.getRetrievalResults()->size(), 3);

  char keys[][4] = {"foo", "baz", "bar"};
  size_t len_vals[] = {14, 4, 4};
  char vals[][15] = {"VALUE foo 0 14", "\r\n \"", "1234"};

  for (i = 0; i < 3; i++) {
    res = &((*parser.getRetrievalResults())[i]);
    innerRes = res->inner();

    size_t len_key = 3;
    ASSERT_N_STREQ(innerRes->key, keys[i], len_key);
    size_t len_val = innerRes->bytes;
    ASSERT_EQ(len_val, len_vals[i]);
    if (len_val > 0) {
      ASSERT_N_STREQ(innerRes->data_block, vals[i], len_val);
    }
  }
}

// TODO test MODE_COUNTING
