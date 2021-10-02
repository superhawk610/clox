#ifndef __CLOX_COMMON_H__
#define __CLOX_COMMON_H__

// #define DEBUG_TRACE_EXEC (print ops/stack to stdout as they're interpreted)
// #define DEBUG_PRINT_CODE (print code to stdout after compilation)
// #define DEBUG_BACKTRACE  (print a backtrace to stderr on segfault)

#define UINT8_COUNT (UINT8_MAX + 1)

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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

// When the background color is changed, some terminals will use that
// color for all empty characters until the end of the line, even if
// it's reset w/ ANSI_Reset. This esc sequence will disable that.
//
// (see https://stackoverflow.com/a/53741857/885098)
#define ANSI_BGFlush   "\e[K"

#endif // __CLOX_COMMON_H__
