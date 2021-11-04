#include <stdio.h>
#include "debug.h"
#include "value.h"
#include "object.h"

void disasm_chunk(Chunk* chunk, const char* name) {
  printf("== %s ==\n", name);

  for (size_t offset = 0; offset < chunk->len; ) {
    offset = disasm_instruction(chunk, offset);
  }

  printf("== /end %s ==\n", name);
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

static size_t byte_instr(const char* name, Chunk* chunk, size_t offset) {
  uint8_t slot = chunk->code[offset + 1];
  printf("%-16s %4d\n", name, slot);

  return offset + 2;
}

static size_t jump_instr(const char* name, int sign, Chunk* chunk, size_t offset) {
  uint16_t jump = (uint16_t) (chunk->code[offset + 1] << 8) | chunk->code[offset + 2];
  printf("%-16s %4zu -> %zu\n", name, offset, offset + 3 + sign * jump);

  return offset + 3;
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

    // -- intrinsic constants --
    case OP_NIL:
      return simple_instr("OP_NIL", offset);
    case OP_TRUE:
      return simple_instr("OP_TRUE", offset);
    case OP_FALSE:
      return simple_instr("OP_FALSE", offset);

    // -- misc. --
    case OP_POP:
      return simple_instr("OP_POP", offset);
    case OP_GET_LOCAL:
      return byte_instr("OP_GET_LOCAL", chunk, offset);
    case OP_SET_LOCAL:
      return byte_instr("OP_SET_LOCAL", chunk, offset);
    case OP_GET_UPVALUE:
      return byte_instr("OP_GET_UPVALUE", chunk, offset);
    case OP_SET_UPVALUE:
      return byte_instr("OP_SET_UPVALUE", chunk, offset);
    case OP_DEF_GLOBAL:
      return const_instr("OP_DEF_GLOBAL", chunk, offset);
    case OP_DEF_GLOBAL_LONG:
      return const_long_instr("OP_DEF_GLOBAL_LONG", chunk, offset);
    case OP_GET_GLOBAL:
      return const_instr("OP_GET_GLOBAL", chunk, offset);
    case OP_GET_GLOBAL_LONG:
      return const_long_instr("OP_GET_GLOBAL_LONG", chunk, offset);
    case OP_SET_GLOBAL:
      return const_instr("OP_SET_GLOBAL", chunk, offset);
    case OP_SET_GLOBAL_LONG:
      return const_long_instr("OP_SET_GLOBAL_LONG", chunk, offset);

    // -- binary ops --
    case OP_ADD:
      return simple_instr("OP_ADD", offset);
    case OP_SUBTRACT:
      return simple_instr("OP_SUBTRACT", offset);
    case OP_MULTIPLY:
      return simple_instr("OP_MULTIPLY", offset);
    case OP_DIVIDE:
      return simple_instr("OP_DIVIDE", offset);
    case OP_EQUAL:
      return simple_instr("OP_EQUAL", offset);
    case OP_GREATER:
      return simple_instr("OP_GREATER", offset);
    case OP_LESS:
      return simple_instr("OP_LESS", offset);

    // -- unary ops --
    case OP_NOT:
      return simple_instr("OP_NOT", offset);
    case OP_NEGATE:
      return simple_instr("OP_NEGATE", offset);

    // -- statements --
    case OP_PRINT:
      return simple_instr("OP_PRINT", offset);

    // -- control flow --
    case OP_JUMP:
      return jump_instr("OP_JUMP", 1, chunk, offset);
    case OP_JUMP_IF_FALSE:
      return jump_instr("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OP_LOOP:
      return jump_instr("OP_LOOP", -1, chunk, offset);
    case OP_CALL:
      return byte_instr("OP_CALL", chunk, offset);
    case OP_CLOSURE: {
      offset++;

      uint8_t constant = chunk->code[offset++];
      printf("%-16s %4d ", "OP_CLOSURE", constant);
      print_value(chunk->constants.values[constant]);
      printf("\n");

      ObjFunction* func = AS_FUNCTION(chunk->constants.values[constant]);
      for (int i = 0; i < func->upvalue_count; i++) {
        bool is_local = chunk->code[offset++] == 1 ? true : false;
        int index = chunk->code[offset++];
        printf("%04zu    |                     %s %d\n",
               offset - 2, is_local ? "local" : "upvalue", index);
      }

      return offset;
    }
    case OP_RETURN:
      return simple_instr("OP_RETURN", offset);

    default:
      printf("unknown opcode %d\n", instr);
      return offset + 1; // advance by a single byte by default
  }
}
