#pragma once
#include <stdlib.h>
#include <stdint.h>

namespace douban {
namespace mc {
namespace hashkit {

typedef uint32_t (*hash_function_t)(const char *key, size_t key_length);

uint32_t hash_md5(const char *key, size_t key_length);
uint32_t hash_fnv1_32(const char *key, size_t key_length);
uint32_t hash_fnv1a_32(const char *key, size_t key_length);
uint32_t hash_crc_32(const char *key, size_t key_length);

} // namespace hashkit
} // namespace mc
} // namespace douban
