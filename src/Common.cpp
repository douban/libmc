#include <cstdarg>

#include "Common.h"


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

// Credits to:
// https://github.com/yandex/ClickHouse/blob/master/libs/libglibc-compatibility/musl/vasprintf.c
#ifndef _GNU_SOURCE
int asprintf(char **s, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int l = vasprintf(s, fmt, ap);
    va_end(ap);
    return l;
}

int vasprintf(char **s, const char *fmt, va_list ap)
{
    va_list ap2;
    va_copy(ap2, ap);
    int l = vsnprintf(0, 0, fmt, ap2);
    va_end(ap2);

    if (l<0 || !(*s=(char *)malloc(l+1U))) return -1;
    return vsnprintf(*s, l+1U, fmt, ap);
}
#endif