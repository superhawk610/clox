#include <stdlib.h>
#include "chunk.h"

void init_chunk(Chunk* chunk) {
  chunk->code = NULL;
  chunk->len = 0;
  chunk->cap = 0;

  init_value_array(&chunk->constants);
  init_rle_array(&chunk->lines);
}

void write_chunk(Chunk* chunk, uint8_t byte, size_t line) {
  if (chunk->len == chunk->cap) {
    size_t old_cap = chunk->cap;
    chunk->cap = GROW_CAPACITY(old_cap);
    chunk->code = GROW_ARRAY(uint8_t, chunk->code, old_cap, chunk->cap);
  }

  chunk->code[chunk->len] = byte;
  push_rle_array(&chunk->lines, line);
  chunk->len++;
}

static size_t add_constant(Chunk* chunk, Value val) {
  write_value_array(&chunk->constants, val);
  return chunk->constants.len - 1;
}

void write_constant(Chunk* chunk, Value val, size_t line) {
  uint16_t constant = add_constant(chunk, val);
  if (constant > UINT8_MAX) {
    write_chunk(chunk, OP_CONST_LONG, line);
    write_chunk(chunk, constant >> 8, line);
    write_chunk(chunk, constant, line);
  } else {
    write_chunk(chunk, OP_CONST, line);
    write_chunk(chunk, constant, line);
  }
}

void free_chunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
  free_value_array(&chunk->constants);
  free_rle_array(&chunk->lines);
  init_chunk(chunk); // leave in a clean, empty state
}
