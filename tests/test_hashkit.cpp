#include <cstdio>
#include <stdint.h>
#include <cstring>
#include <string>
#include <fstream>
#include "Common.h"
#include "hashkit/md5.h"
#include "hashkit/hashkit.h"
#include "gtest/gtest.h"
#include "test_common.h"


using std::atoi;
using std::string;
using std::getline;
using std::ifstream;
using std::stringstream;
using douban::mc::tests::get_resource_path;


static const char UTF8_ZHONG[] = "\xe4\xb8\xad";  // "中" in UTF-8


void test_md5_hexdigest(const char key[], size_t key_len, char hexdigest[33]) {
  const unsigned char* input = reinterpret_cast<const unsigned char *>(key);
  unsigned char out[16];
  douban::mc::hashkit::md5(input, key_len, out);
  int i = 0;
  char hexdigest2[33] = {0};
  for (i = 0; i < 16; i++) {
    snprintf(hexdigest2 + i * 2, 3, "%02x", out[i]);
  }
  ASSERT_N_STREQ(hexdigest, hexdigest2, 32);
}


TEST(hashkit, md5_string) {
  test_md5_hexdigest("", 0U, CSTR("d41d8cd98f00b204e9800998ecf8427e"));
  test_md5_hexdigest("123456", 6U, CSTR("e10adc3949ba59abbe56e057f20f883e"));
  test_md5_hexdigest("abcdef", 6U, CSTR("e80b5017098950fc58aad83c8c14978e"));
  test_md5_hexdigest(UTF8_ZHONG, 3U, CSTR("aed1dfbc31703955e64806b799b67645"));
}


TEST(hashkit, fnv1_32) {
  // test data are generated using:
  // http://www.isthe.com/chongo/src/fnv/fnv-5.0.3.tar.gz
  // echo -ne "abcdef" | ./fnv132

  ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32("", 0U), 0x811c9dc5U);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32("123456", 6U), 0xeb008bb8U);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32("abcdef", 6U), 0x9f2d4718);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32(UTF8_ZHONG, 3U), 0x6cd2e676);

  // SOLVED! What is the shortest binary data set for which the 32-bit FNV-1 hash is 0?
  // 32-bit solution by Russ Cox <rcs Email address> on 2011-Mar-02:
  // There are 254 solutions of length 5, ranging from
  // FNV32_1(01 47 6c 10 f3) = 0
  // to
  // FNV32_1(fd 45 41 08 a0) = 0
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32("\x01\x47\x6c\x10\xf3", 5U), 0U);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32("\xfd\x45\x41\x08\xa0", 5U), 0U);
}


TEST(hashkit, fnv1a_32) {
  // test data are generated using:
  // http://www.isthe.com/chongo/src/fnv/fnv-5.0.3.tar.gz
  // echo -ne "abcdef" | ./fnv1a32

  ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32("", 0U), 0x811c9dc5U);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32("123456", 6U), 0x9995b6aaU);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32("abcdef", 6U), 0xff478a2aU);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32(UTF8_ZHONG, 3U), 0xb846d3f4U);

  // SOLVED! What is the shortest binary data set for which the 32-bit FNV-1a hash is 0?
  // 32-bit solution by Russ Cox <rcs Email address> on 2011-Mar-02:
  // The solutions of length 4 are:
  //
  // FNV32_1a(cc 24 31 c4) = 0
  // FNV32_1a(e0 4d 9f cb) = 0
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32("\xcc\x24\x31\xc4", 4U), 0U);
  ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32("\xe0\x4d\x9f\xcb", 4U), 0U);
}


TEST(hashkit, crc_32) {
  // test data are generated via:
  // hex(np.uint32(zlib.crc32(key)))
  ASSERT_EQ(douban::mc::hashkit::hash_crc_32("", 0U), 0x0U);
  ASSERT_EQ(douban::mc::hashkit::hash_crc_32("123456", 6U), 0x972d361U);
  ASSERT_EQ(douban::mc::hashkit::hash_crc_32("abcdef", 6U), 0x4b8e39efU);
  ASSERT_EQ(douban::mc::hashkit::hash_crc_32(UTF8_ZHONG, 3U), 0xd5d15a8bU);
}


TEST(hashkit, mass) {
  std::string keys_path = get_resource_path("keys.txt");
  std::string keys_crc_32_path = get_resource_path("keys_crc_32.txt");
  std::string keys_fnv1_32_path = get_resource_path("keys_fnv1_32.txt");
  std::string keys_fnv1a_32_path = get_resource_path("keys_fnv1a_32.txt");
  std::string keys_md5_path = get_resource_path("keys_md5.txt");

  ifstream keys_stream(keys_path.c_str());
  ifstream keys_crc_32_stream(keys_crc_32_path.c_str());
  ifstream keys_fnv1_32_stream(keys_fnv1_32_path.c_str());
  ifstream keys_fnv1a_32_stream(keys_fnv1a_32_path.c_str());
  ifstream keys_md5_stream(keys_md5_path.c_str());

  string key_line;
  uint32_t key_crc_32, key_fnv1_32, key_fnv1a_32, key_md5;
  ASSERT_TRUE(keys_stream.good());
  ASSERT_TRUE(keys_crc_32_stream.good());
  ASSERT_TRUE(keys_fnv1_32_stream.good());
  ASSERT_TRUE(keys_fnv1a_32_stream.good());
  ASSERT_TRUE(keys_md5_stream.good());

  while (getline(keys_stream, key_line)) {
    keys_crc_32_stream >> key_crc_32;
    keys_fnv1_32_stream >> key_fnv1_32;
    keys_fnv1a_32_stream >> key_fnv1a_32;
    keys_md5_stream >> key_md5;

    ASSERT_EQ(douban::mc::hashkit::hash_crc_32(key_line.c_str(), key_line.length()), key_crc_32);
    ASSERT_EQ(douban::mc::hashkit::hash_fnv1_32(key_line.c_str(), key_line.length()), key_fnv1_32);
    ASSERT_EQ(douban::mc::hashkit::hash_fnv1a_32(key_line.c_str(), key_line.length()), key_fnv1a_32);
    ASSERT_EQ(douban::mc::hashkit::hash_md5(key_line.c_str(), key_line.length()), key_md5);
  }
  keys_stream.close();
  keys_crc_32_stream.close();
  keys_fnv1_32_stream.close();
  keys_fnv1a_32_stream.close();
  keys_md5_stream.close();
}
