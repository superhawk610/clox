#include <stdlib.h>
#include "chunk.h"

void init_chunk(Chunk* chunk) {
  chunk->code = NULL;
  chunk->lines = NULL;
  chunk->len = 0;
  chunk->cap = 0;

  init_value_array(&chunk->constants);
}

void write_chunk(Chunk* chunk, uint8_t byte, int line) {
  if (chunk->len == chunk->cap) {
    int old_cap = chunk->cap;
    chunk->cap = GROW_CAPACITY(old_cap);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->cap);
    chunk->lines = GROW_ARRAY(int, chunk->lines, old_cap, chunk->cap);
  }

  chunk->code[chunk->len] = byte;
  chunk->lines[chunk->len] = line;
  chunk->len++;
}

int add_constant(Chunk* chunk, Value val) {
  write_value_array(&chunk->constants, val);
  return chunk->constants.len - 1;
}

void free_chunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
  FREE_ARRAY(int, chunk->lines, chunk->cap);
  free_value_array(&chunk->constants);
  init_chunk(chunk); // leave in a clean, empty state
}
