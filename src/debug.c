#include <stdio.h>
#include "debug.h"
#include "value.h"

void disasm_chunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (int offset = 0; offset < chunk->len; ) {
    offset = disasm_instruction(chunk, offset);
  }
}

static int const_instruction(const char* name, Chunk* chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");

  return offset + 2; // we consume the instr and const value bytes
}

static int simple_instruction(const char* name, int offset) {
  printf("%s\n", name);

  return offset + 1;
}

int disasm_instruction(Chunk* chunk, int offset) {
  printf("%04d ", offset); // print instruction byte address

  // print line number (use `|` for runs of same line no.)
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instr = chunk->code[offset];
  switch (instr) {
    case OP_CONST:
      return const_instruction("OP_CONST", chunk, offset);
    case OP_RETURN:
      return simple_instruction("OP_RETURN", offset);
    default:
      printf("unknown opcode %d\n", instr);
      return offset + 1; // advance by a single byte by default
  }
}
