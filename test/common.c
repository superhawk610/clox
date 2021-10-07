#include <stdlib.h>
#include <string.h>
#include "common.h"

void testing__assert_fail(char* expr,
                   const char* __file,
                   unsigned int __line,
                   const char* __function) {
  fprintf(stderr, "\n" ANSI_BGRed ANSI_Black "  \u2718  " ANSI_Reset \
                  " assertion failed " ANSI_Cyan "[%s]" ANSI_Reset \
                  ANSI_Dim " (%s:%d):" ANSI_Reset "\n%s\n",
                  __function, __file, __line, expr);

  exit(1);
}

char* duplicate_string(const char* str) {
  size_t len = strlen(str);
  char* dup = (char*) malloc(len + 1);
  strcpy(dup, str);
  return dup;
}
