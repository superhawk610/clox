#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__

#include <stdlib.h>
#include <stdio.h>

#define ANSI_Reset     "\033[0m"
#define ANSI_Dim       "\033[2m"

#define ANSI_Black     "\033[30m"
#define ANSI_Red       "\033[31m"
#define ANSI_Green     "\033[32m"
#define ANSI_Yellow    "\033[33m"
#define ANSI_Blue      "\033[34m"
#define ANSI_Magenta   "\033[35m"
#define ANSI_Cyan      "\033[36m"
#define ANSI_White     "\033[37m"

#define ANSI_BGBlack   "\033[40m"
#define ANSI_BGRed     "\033[41m"
#define ANSI_BGGreen   "\033[42m"
#define ANSI_BGYellow  "\033[43m"
#define ANSI_BGBlue    "\033[44m"
#define ANSI_BGMagenta "\033[45m"
#define ANSI_BGCyan    "\033[46m"
#define ANSI_BGWhite   "\033[47m"

void testing__assert_fail(char* expr,
                   const char* __file,
                   unsigned int __line,
                   const char* __function);

#define __func __extension__ __PRETTY_FUNCTION__

#define assert(expr) \
  ((expr) \
   ? (void) (0) \
   : testing__assert_fail("assert " #expr, __FILE__, __LINE__, __func))

#define refute(expr) \
  ((expr) \
   ? testing__assert_fail("refute " #expr, __FILE__, __LINE__, __func) \
   : (void) (0))

char* duplicate_string(const char* str);

#endif // __TEST_COMMON_H__
