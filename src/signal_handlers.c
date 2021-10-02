#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "signal_handlers.h"

#define EX_SEGFAULT 139

static void segv_handler(int signal) {
#ifdef DEBUG_BACKTRACE
  void* call_stack[10];
  size_t stack_depth;

  stack_depth = backtrace(call_stack, 10);

  fprintf(stderr, ANSI_BGRed ANSI_Black "\n Segmentation fault (SIG %d) " ANSI_Reset ANSI_BGFlush "\n\n", signal);
  fprintf(stderr, ANSI_Dim "  Stacktrace:" ANSI_Reset "\n");

  char** symbols = backtrace_symbols(call_stack, stack_depth);
  for (size_t i = 0; i < stack_depth; i++) {
    fprintf(stderr, "  %s\n", symbols[i]);
  }
#endif

  exit(EX_SEGFAULT);
}

void install_signal_handlers() {
  signal(SIGSEGV, segv_handler);
}

// ---

#undef EX_SEGFAULT
