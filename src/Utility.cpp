#include "Utility.h"
#include "Common.h"

namespace douban {
namespace mc {
namespace utility {

bool isValidKey(const char* key, const size_t keylen) {

#define HANDLE_BAD_MC_KEY() \
  do { \
    log_warn("invalid mc key of length %zu: \"%.*s\"", keylen, static_cast<int>(keylen), key); \
    return false; \
  } while (0)

  if (keylen > MC_MAX_KEY_LENGTH) {
    HANDLE_BAD_MC_KEY();
  }

  for (size_t i = 0; i < keylen; i++) {
    switch (key[i]) {
      case ' ':
        HANDLE_BAD_MC_KEY();
        break;
      case '\r':
        HANDLE_BAD_MC_KEY();
        break;
      case '\n':
        HANDLE_BAD_MC_KEY();
        break;
      case 0:
        HANDLE_BAD_MC_KEY();
        break;
      default:
        break;
    }
  }
  return true;
}


// credits for: https://gist.github.com/sergot/1333837
void fprintBuffer(std::FILE* file, const char *data_buffer_, const unsigned int length) {
  const unsigned char* data_buffer = reinterpret_cast<const unsigned char*>(data_buffer_);
  unsigned int i, j;
  for (i = 0; i < length; i++) {
    unsigned char byte = data_buffer[i];
    fprintf(file, "%02x ", data_buffer[i]);
    if (((i%16) == 15) || (i == length-1)) {
      for (j = 0; j < 15-(i%16); j++) {
        fprintf(file, "   ");
      }
      fprintf(file, "| ");
      for (j=(i-(i%16)); j <= i; j++) {
        byte = data_buffer[j];
        if ((byte > 31) && (byte < 127)) {
          fprintf(file, "%c", byte);
        } else {
          fprintf(file, ".");
        }
      }
      fprintf(file, "\n");
    }
  }
}


} // namespace utility
} // namespace mc
} // namespace douban
