#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "repl.h"
#include "vm.h"

static char* read_file(const char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "could not open file \"%s\".\n", path);
    exit(EX_IOERR);
  }

  fseek(f, 0L, SEEK_END);
  size_t f_size = ftell(f);
  rewind(f);

  char* buf = (char*) malloc(f_size + 1);
  if (f == NULL) {
    fprintf(stderr, "not enough memory to read \"%s\".\n", path);
    exit(EX_IOERR);
  }

  size_t n_read = fread(buf, sizeof(buf), f_size, f);
  if (n_read < f_size) {
    fprintf(stderr, "could not read file \"%s\".\n", path);
    exit(EX_IOERR);
  }

  buf[n_read] = '\0';

  fclose(f);

  return buf;
}

static void run_file(const char* path) {
  char* source = read_file(path);
  InterpretResult res = interpret(source);
  free(source);

  if (res == INTERPRET_COMPILE_ERR) exit(EX_DATAERR);
  if (res == INTERPRET_RUNTIME_ERR) exit(EX_SOFTWARE);
}

#ifndef __TESTING__
int main(int argc, const char* argv[]) {
#define LINE_NO 123

  init_vm();

  if      (argc == 1) repl();
  else if (argc == 2) run_file(argv[1]);
  else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(EX_USAGE);
  }

  free_vm();

  return 0;

#undef LINE_NO
}
#endif
