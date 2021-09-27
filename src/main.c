#include <stdio.h>
#include <stdlib.h>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void repl() {
  char line[1024];

  for (;;) {
    printf("> ");

    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }

    interpret(line);
  }
}

static char* read_file(const char* path) {
  FILE* f = fopen(path, "rb");
  if (f == NULL) {
    fprintf(stderr, "could not open file \"%s\".\n", path);
    exit(74);
  }

  fseek(f, 0L, SEEK_END);
  size_t f_size = ftell(f);
  rewind(f);

  char* buf = (char*) malloc(f_size + 1);
  if (f == NULL) {
    fprintf(stderr, "not enough memory to read \"%s\".\n", path);
    exit(74);
  }

  size_t n_read = fread(buf, sizeof(buf), f_size, f);
  if (n_read < f_size) {
    fprintf(stderr, "could not read file \"%s\".\n", path);
    exit(74);
  }

  buf[n_read] = '\0';

  fclose(f);

  return buf;
}

static void run_file(const char* path) {
  char* source = read_file(path);
  InterpretResult res = interpret(source);
  free(source);

  if (res == INTERPRET_COMPILE_ERR) exit(65);
  if (res == INTERPRET_RUNTIME_ERR) exit(70);
}

int main(int argc, const char* argv[]) {
#define LINE_NO 123

  // Chunk chunk;

  init_vm();

  if      (argc == 1) repl();
  else if (argc == 2) run_file(argv[1]);
  else {
    fprintf(stderr, "Usage: clox [path]\n");
    exit(64);
  }

  // init_chunk(&chunk);

  // write_constant(&chunk, 1.23, LINE_NO);
  // write_constant(&chunk, 2.34, LINE_NO);
  // write_chunk(&chunk, OP_NEGATE, LINE_NO);
  // write_chunk(&chunk, OP_ADD, LINE_NO);
  // write_constant(&chunk, 2, LINE_NO);
  // write_chunk(&chunk, OP_DIVIDE, LINE_NO);
  // write_chunk(&chunk, OP_NEGATE, LINE_NO);
  // write_chunk(&chunk, OP_RETURN, LINE_NO);

  // disasm_chunk(&chunk, "test chunk");

  // interpret(&chunk);

  free_vm();

  // free_chunk(&chunk);

  return 0;

#undef LINE_NO
}
