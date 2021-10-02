#ifndef __CLOX_COMMON_H__
#define __CLOX_COMMON_H__

// #define DEBUG_TRACE_EXEC
// #define DEBUG_PRINT_CODE

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

#endif // __CLOX_COMMON_H__
