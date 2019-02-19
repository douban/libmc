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

} // namespace mc
} // namespace douban
