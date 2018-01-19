#include "Common.h"
#ifdef __GLIBC__
#include <execinfo.h>
#include <cstdlib>

std::shared_ptr<spdlog::logger> stderr_logger = spdlog::stderr_logger_mt(PROJECT_NAME);

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
