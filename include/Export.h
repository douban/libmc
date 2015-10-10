#pragma once

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif


typedef int64_t exptime_t;
typedef uint32_t flags_t;
typedef uint64_t cas_unique_t;


typedef struct {
  char* host;
  char** lines;
  size_t* line_lens;
  size_t len;
} broadcast_result_t;


typedef struct {
  char* key; // 8B
  char* data_block; // 8B
  cas_unique_t cas_unique; // 8B
  uint32_t bytes; // 4B
  flags_t flags;  // 4B
  uint8_t key_len; // 1B
} retrieval_result_t;
