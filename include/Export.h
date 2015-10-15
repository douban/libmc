#pragma once

#include <stdint.h>
#ifdef __cplusplus
#include <cstdlib>
#else
#include <stdlib.h>
#endif


typedef enum {
  CFG_POLL_TIMEOUT,
  CFG_CONNECT_TIMEOUT,
  CFG_RETRY_TIMEOUT,
  CFG_HASH_FUNCTION
} config_options_t;


typedef enum {
  OPT_HASH_MD5,
  OPT_HASH_FNV1_32,
  OPT_HASH_FNV1A_32,
  OPT_HASH_CRC_32,
} hash_function_options_t;


typedef enum {
  RET_SEND_ERR = -9,
  RET_RECV_ERR = -8,
  RET_CONN_POLL_ERR = -7,
  RET_POLL_TIMEOUT_ERR = -6,
  RET_POLL_ERR = -5,
  RET_MC_SERVER_ERR = -4,
  RET_PROGRAMMING_ERR = -3,
  RET_INVALID_KEY_ERR = -2,
  RET_INCOMPLETE_BUFFER_ERR = -1,
  RET_OK = 0
} err_code_t;


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


enum message_result_type {
  MSG_EXISTS,
  MSG_OK,
  MSG_STORED,
  MSG_NOT_STORED,
  MSG_NOT_FOUND,
  MSG_DELETED,
  MSG_TOUCHED,
};


typedef struct {
  enum message_result_type type_;
  char* key;
  size_t key_len;
} message_result_t;


typedef struct {
  char* key;
  size_t key_len;
  uint64_t value;
} unsigned_result_t;
