#pragma once

#include <sys/uio.h>
#include <poll.h>
#include <stdint.h>

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cerrno>

#define PROJECT_NAME "libmc"
#define MC_DEFAULT_PORT 11211
#define MC_DEFAULT_POLL_TIMEOUT 300
#define MC_DEFAULT_CONNECT_TIMEOUT 10
#define MC_DEFAULT_RETRY_TIMEOUT 5


#ifdef UIO_MAXIOV
#define MC_UIO_MAXIOV UIO_MAXIOV
#else
#define MC_UIO_MAXIOV 1024
#endif


#ifdef NI_MAXHOST
#define MC_NI_MAXHOST NI_MAXHOST
#else
#define MC_NI_MAXHOST 1025
#endif

#ifdef NI_MAXSERV
#define MC_NI_MAXSERV NI_MAXSERV
#else
#define MC_NI_MAXSERV 32
#endif

#ifndef MSG_MORE
#define MC_MSG_MORE 0
#else
#define MC_MSG_MORE MSG_MORE
#endif

#define MC_BUFFER_SIZE 8192
#define RECV_BUFFER_SIZE 8192
#define MIN(A, B) (((A) > (B)) ? (B) : (A))
#define MAX(A, B) (((A) < (B)) ? (B) : (A))
#define CSTR(STR) (const_cast<char*>(STR))
#define ASSERT_N_STREQ(S1, S2, N) do {ASSERT_TRUE(0 == std::strncmp((S1), (S2), (N)));} while (0)

#if defined __cplusplus
# define __VOID_CAST static_cast<void>
#else
# define __VOID_CAST (void)
#endif


#define _mc_clean_errno() (errno == 0 ? "None" : strerror(errno))
#define _mc_output_stderr(LEVEL, FORMAT, ...) ( \
  fprintf( \
    stderr, \
    "[" PROJECT_NAME "] [" #LEVEL "] [%s:%u] " FORMAT "\n", \
    __FILE__, __LINE__, ##__VA_ARGS__ \
  ), \
  __VOID_CAST(0) \
)

void printBacktrace();
#ifdef NDEBUG
#define debug(M, ...) __VOID_CAST(0)

#define _ASSERTION_FAILED(cond) ( \
  _mc_output_stderr(PANIC, "failed assertion `%s'" , #cond), \
  printBacktrace(), \
  __VOID_CAST(0) \
)
#else
#define debug(M, ...) ( \
  _mc_output_stderr(DEBUG, "[E: %s] " M, _mc_clean_errno(), ##__VA_ARGS__), \
  __VOID_CAST(0) \
)

#define _ASSERTION_FAILED(cond) ( \
  _mc_output_stderr(PANIC, "failed assertion `%s'" , #cond), \
  printBacktrace(), \
  abort() \
)
#endif

#define log_info(M, ...) ( \
  _mc_output_stderr(INFO, M, ##__VA_ARGS__), \
  __VOID_CAST(0) \
)

#define log_warn(M, ...) ( \
  _mc_output_stderr(WARN, "[E: %s] " M, _mc_clean_errno(), ##__VA_ARGS__), \
  __VOID_CAST(0) \
)

#define log_err(M, ...) ( \
  _mc_output_stderr(ERROR, "[E: %s] " M, _mc_clean_errno(), ##__VA_ARGS__), \
  __VOID_CAST(0) \
)

#define ASSERT(cond) ( \
  (cond) ? \
  __VOID_CAST(0) : \
  _ASSERTION_FAILED(cond) \
)

#define NOT_REACHED() ASSERT(0)


namespace douban {
namespace mc {

typedef struct pollfd pollfd_t;

// FSM: finite state machine
typedef enum {
  FSM_START,

  // END
  FSM_END, // got "END\r\n"

  // ERROR
  FSM_ERROR, // got "ERROR\r\n"

  // VALUE
  FSM_GET_START, // got "VALUE "
  FSM_GET_KEY, // got "key "
  FSM_GET_FLAG, // got "flag "
  FSM_GET_BYTES_CAS, // got "bytes cas\r" or got "bytes "
  FSM_GET_VALUE_REMAINING, // got last "\r\n" not got all bytes + "\r\n"

  // VERSION
  FSM_VER_START, // got "VERSION "

  // STAT
  FSM_STAT_START, // got "STAT "

  // [0-9] // INCR/DESC
  FSM_INCR_DECR_START, // got [0-9]
  FSM_INCR_DECR_REMAINING, // not got "\r\n"
} parser_state_t;

#define IS_END_STATE(st) ((st) == FSM_END or (st) == FSM_ERROR)


typedef enum {
  // storage commands -> key_va -> success_or_failure
  // key -> flags_t -> exptime_t -> bytes -> [cas_unique_t] -> success_or_fail
  SET_OP,
  ADD_OP,
  REPLACE_OP,
  APPEND_OP,
  PREPEND_OP,
  CAS_OP,

  // retrieval commands -> key(s) -> key_val(s)
  // key(s) ->
  GET_OP,
  GETS_OP,

  // modify commands -> key -> uint64_t -> uint64_t_or_not_found
  INCR_OP,
  DECR_OP,

  // touch command
  TOUCH_OP, // key -> exptime_t -> text_msg

  // key commands -> key -> text_msg
  DELETE_OP, // key -> text_msg
  STATS_OP,
  FLUSHALL_OP,
  VERSION_OP,
  QUIT_OP,
} op_code_t;


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

} // namespace mc
} // namespace douban
