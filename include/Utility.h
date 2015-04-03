#pragma once

#include <stdint.h>
#include <cstddef>
#include <cstdio>
#include "rapidjson/itoa.h"

#define MC_MAX_KEY_LENGTH 250

namespace douban {
namespace mc {
namespace utility {

inline int int64ToCharArray(int64_t n, char s[]) {
  char *end = ::rapidjson::internal::i64toa(n, s);
  *end = '\0';
  return static_cast<int>(end - s);
}

// credit to The New Page of Injections Book:
// Memcached Injections @ blackhat2014 [pdf](http://t.cn/RP0J10Z)
bool isValidKey(const char* key, const size_t keylen);
void fprintBuffer(std::FILE* file, const char *data_buffer_, const unsigned int length);

} // namespace utility
} // namespace mc
} // namespace douban
