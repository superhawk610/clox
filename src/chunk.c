#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
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

uint16_t add_constant(Chunk* chunk, Value val) {
  if (chunk->constants.len == UINT16_MAX) {
    fprintf(stderr, "unable to store more than %u constants in chunk", UINT16_MAX);
    exit(EX_SOFTWARE);
  }

  value_array_push(&chunk->constants, val);
  return chunk->constants.len - 1;
}

void free_chunk(Chunk* chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
  free_value_array(&chunk->constants);
  free_rle_array(&chunk->lines);
  init_chunk(chunk); // leave in a clean, empty state
}
