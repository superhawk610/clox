#ifndef __CLOX_LOGGER_H__
#define __CLOX_LOGGER_H__

#include <stdio.h>

void init_logger();

void out_printf(const char* fmt, ...);

void err_printf(const char* fmt, ...);

void logger_redirect_out(FILE* stream);

void logger_redirect_err(FILE* stream);

void logger_restore_out();

void logger_restore_err();

#endif // __CLOX_LOGGER_H__
