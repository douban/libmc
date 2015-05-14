#include "hashkit/hashkit.h"

// http://www.isthe.com/chongo/src/fnv/hash_32a.c

namespace douban {
namespace mc {
namespace hashkit {

static const uint32_t FNV_32_INIT = 2166136261UL;

#if defined(NO_FNV_GCC_OPTIMIZATION)
static const uint32_t FNV_32_PRIME = 16777619UL;
#endif

uint32_t hash_fnv1_32(const char *key, size_t key_length) {
    uint32_t hash = FNV_32_INIT;
    size_t x;
    const uint8_t* unsigned_key = reinterpret_cast<const uint8_t*>(key);

    for (x = 0; x < key_length; x++) {
      uint32_t val = (uint32_t)unsigned_key[x];
#if defined(NO_FNV_GCC_OPTIMIZATION)
      hash *= FNV_32_PRIME;
#else
      hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
#endif
      hash ^= val;
    }

    return hash;
}

uint32_t hash_fnv1a_32(const char *key, size_t key_length) {
    uint32_t hash = FNV_32_INIT;
    size_t x;
    const uint8_t* unsigned_key = reinterpret_cast<const uint8_t*>(key);

    for (x= 0; x < key_length; x++) {
      uint32_t val = (uint32_t)unsigned_key[x];
      hash ^= val;
#if defined(NO_FNV_GCC_OPTIMIZATION)
      hash *= FNV_32_PRIME;
#else
      hash += (hash << 1) + (hash << 4) + (hash << 7) + (hash << 8) + (hash << 24);
#endif
    }

    return hash;
}

} // namespace hashkit
} // namespace mc
} // namespace douban
