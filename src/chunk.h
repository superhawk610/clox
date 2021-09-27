#ifndef __CLOX_CHUNK_H__
#define __CLOX_CHUNK_H__

#include "common.h"
#include "value.h"
#include "memory.h"

typedef enum {
  OP_CONST,
  OP_RETURN,
} OpCode;

typedef struct {
  // dynamic array containing all bytes in program bytecode
  uint8_t* code;

  int len; // number of bytes in chunk
  int cap; // available bytes in chunk

  // block of memory for storing constant values
  ValueArray constants;

  // dynamic array containing line no. info, matching the
  // `len` and `cap` of `code`
  int* lines;
} Chunk;

void init_chunk(Chunk* chunk);

void write_chunk(Chunk* chunk, uint8_t byte, int line);

int add_constant(Chunk* chunk, Value val);

void free_chunk(Chunk* chunk);

#endif // __CLOX_CHUNK_H__
