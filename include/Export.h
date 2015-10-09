#pragma once

#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif

typedef struct {
  char* host;
  char** lines;
  size_t* line_lens;
  size_t len;
} broadcast_result_t;
