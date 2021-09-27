#include <stdio.h>
#include "debug.h"
#include "value.h"

void disasm_chunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (size_t offset = 0; offset < chunk->len; ) {
    offset = disasm_instruction(chunk, offset);
  }
}

static size_t const_instr(const char* name, Chunk* chunk, size_t offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");

  return offset + 2; // we consume the instr and const value bytes
}

static size_t const_long_instr(const char* name, Chunk* chunk, size_t offset) {
  uint16_t constant = ((uint16_t) chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
  printf("%-16s %4d '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");

  return offset + 3; // we consume the instr and 2x const value bytes
}

static size_t simple_instr(const char* name, size_t offset) {
  printf("%s\n", name);

  return offset + 1;
}

size_t disasm_instruction(Chunk* chunk, size_t offset) {
#define NTH_LINE_NO(chunk, offset) \
  get_nth_rle_array(&chunk->lines, offset)

  printf("%04zu ", offset); // print instruction byte address

  // print line number (use `|` for runs of same line no.)
  size_t curr_line = get_nth_rle_array(&chunk->lines, offset);
  if (offset > 0 && curr_line == get_nth_rle_array(&chunk->lines, offset - 1)) {
    printf("   | ");
  } else {
    printf("%4zu ", curr_line);
  }

  uint8_t instr = chunk->code[offset];
  switch (instr) {
    case OP_CONST:
      return const_instr("OP_CONST", chunk, offset);
    case OP_CONST_LONG:
      return const_long_instr("OP_CONST_LONG", chunk, offset);
    case OP_RETURN:
      return simple_instr("OP_RETURN", offset);
    default:
      printf("unknown opcode %d\n", instr);
      return offset + 1; // advance by a single byte by default
  }
}
