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


char* string_format(const char *fmt, ...)
{
    int size = 0;
    va_list args;

    // init variadic argumens
    va_start(args, fmt);

    va_list args2;

    // copy
    va_copy(args2, args);

    // apply variadic arguments to
    // sprintf with format to get size
    size = snprintf(NULL, size, fmt, args2);

    va_end(args2);

    if (size < 0) { return NULL; }

    // alloc with size plus 1 for `\0'
    char* str = new char[size + 1];

    if (NULL == str) { return NULL; }

    // format string with original
    // variadic arguments and set new size
    if (snprintf(str, size, fmt, args) != size) {
        delete[] str;
        str = NULL;
    }
    va_end(args);
    return str;
}