#include <stdarg.h>
#include "logger.h"

static FILE* out_stream;
static FILE* err_stream;

void init_logger() {
  out_stream = stdout;
  err_stream = stderr;
}

void out_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(out_stream, fmt, args);
  va_end(args);
}

void err_printf(const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  vfprintf(err_stream, fmt, args);
  va_end(args);
}

void logger_redirect_out(FILE* stream) { out_stream = stream; }

void logger_redirect_err(FILE* stream) { err_stream = stream; }

void logger_restore_out() { out_stream = stdout; }

void logger_restore_err() { err_stream = stderr; }
