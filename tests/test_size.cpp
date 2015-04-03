#include <stdio.h>
#include "BufferReader.h"
#include "Result.h"
#include "gtest/gtest.h"


TEST(test_size, main) {

#ifdef MC_USE_SMALL_VECTOR
  ASSERT_EQ(sizeof(douban::mc::io::TokenData), 48U);
#else
  ASSERT_EQ(sizeof(douban::mc::io::TokenData), 24U);
#endif
  ASSERT_EQ(sizeof(douban::mc::io::TokenData*), 8U);
}
