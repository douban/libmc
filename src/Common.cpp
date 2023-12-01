#include "Common.h"
#include "Keywords.h"

#ifdef __GLIBC__
#include <execinfo.h>
#include <cstdlib>


void printBacktrace() {
  void *trace_elems[20];
  int trace_elem_count(backtrace(trace_elems, 20));
  char **stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
  for ( int i = 0 ; i < trace_elem_count ; ++i ) {
    fprintf(stderr, "    %s\n", stack_syms[i]);
  }
  free(stack_syms);
}
#else
void printBacktrace() {}
#endif

namespace douban {
namespace mc {

const char* errCodeToString(err_code_t err) {
  switch (err)
  {
    case RET_SEND_ERR:
      return keywords::kSEND_ERROR;
    case RET_RECV_ERR:
      return keywords::kRECV_ERROR;
    case RET_CONN_POLL_ERR:
      return keywords::kCONN_POLL_ERROR;
    case RET_POLL_TIMEOUT_ERR:
      return keywords::kPOLL_TIMEOUT_ERROR;
    case RET_POLL_ERR:
      return keywords::kPOLL_ERROR;
    case RET_MC_SERVER_ERR:
      return keywords::kSERVER_ERROR;
    case RET_PROGRAMMING_ERR:
      return keywords::kPROGRAMMING_ERROR;
    case RET_INVALID_KEY_ERR:
      return keywords::kINVALID_KEY_ERROR;
    case RET_INCOMPLETE_BUFFER_ERR:
      return keywords::kINCOMPLETE_BUFFER_ERROR;
    case RET_OK:
      return "ok";
    default:
      return "unknown";
  }
}

bool isLocalSocket(const char* host) {
  // errors on the side of false negatives, allowing syntax expansion;
  // starting slash to denote socket paths is from pylibmc
  return host[0] == '/';
}

void verifyPort(const char* input, ServerSpec* res, bool* valid_port) {
  /*
  if (*valid_port && input > res->port) {
    res->port[-1] = '\0';
  } else if (res->alias == NULL) {
    res->port = NULL;
  }
  *valid_port = false;
  */
  if (res->port != NULL) {
    res->port[-1] = '\0';
  }
}

// modifies input string and output pointers reference input
ServerSpec splitServerString(char* input) {
  bool escaped = false, valid_port = false;
  ServerSpec res = { input, NULL, NULL };
  for (;; input++) {
    switch (*input)
    {
      case '\0':
        verifyPort(input, &res, &valid_port);
        return res;
      case ':':
        if (res.alias == NULL) {
          res.port = input + 1;
          valid_port = true;
        }
        escaped = false;
        continue;
      case ' ':
        if (!escaped) {
          *input = '\0';
          if (res.alias == NULL) {
            res.alias = input + 1;
            verifyPort(input, &res, &valid_port);
            continue;
          } else {
            return res;
          }
        }
      default:
        valid_port = false;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9': 
        escaped = false;
        continue;
      case '\\':
        escaped ^= 1;
        valid_port = false;
    }
  }
}

} // namespace mc
} // namespace douban
