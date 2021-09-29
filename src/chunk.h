#ifndef __CLOX_CHUNK_H__
#define __CLOX_CHUNK_H__

#include "common.h"
#include "value.h"
#include "memory.h"
#include "rle_array.h"

typedef enum {
  OP_CONST,
  OP_CONST_LONG,

  // -- binary ops --
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,

  // -- unary ops --
  OP_NEGATE,

  OP_RETURN,
} OpCode;

typedef struct {
  // dynamic array containing all bytes in program bytecode
  uint8_t* code;

  size_t len; // number of bytes in chunk
  size_t cap; // available bytes in chunk

  // block of memory for storing constant values
  ValueArray constants;

  // run-length encoded array containing line no. info
  RLEArray lines;
} Chunk;

void init_chunk(Chunk* chunk);

void write_chunk(Chunk* chunk, uint8_t byte, size_t line);

uint16_t add_constant(Chunk* chunk, Value val);

void free_chunk(Chunk* chunk);

#endif // __CLOX_CHUNK_H__
