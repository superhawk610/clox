#include <stdarg.h>
#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "vm.h"

VM vm; // global singleton, since we don't support parallel VMs

static void reset_stack() {
  vm.stack_top = vm.stack;
}

void init_vm() {
  reset_stack();
}

void free_vm() {
}

static Value peek(size_t distance);

static void runtime_error(const char* format, ...) {
  va_list args; // song and dance to get variadic args
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputs("\n", stderr);

  size_t instruction = vm.ip - vm.chunk->code - 1;
  size_t line = get_nth_rle_array(&vm.chunk->lines, instruction);
  fprintf(stderr, "[line %zu] in script\n", line);
  reset_stack();
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONST() (vm.chunk->constants.values[READ_BYTE()])

#define BINARY_OP(value_type, op) \
  do { \
    if (!IS_NUMBER(peek(0)) || !IS_NUMBER(peek(1))) { \
      runtime_error("Operands must be numbers."); \
      return INTERPRET_RUNTIME_ERR; \
    } \
    double b = AS_NUMBER(pop()); \
    double a = AS_NUMBER(pop()); \
    push(value_type(a op b)); \
  } while (0)

  for (;;) {
#ifdef DEBUG_TRACE_EXEC
    // display current stack
    printf("          ");
    for (Value* slot = vm.stack; slot < vm.stack_top; slot++) {
      printf("[ ");
      print_value(*slot);
      printf(" ]");
    }
    printf("\n");

    // display instruction
    disasm_instruction(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
#endif

    uint8_t instr;
    switch(instr = READ_BYTE()) {
      case OP_CONST: {
        Value constant = READ_CONST();
        push(constant);

        break;
      }

      // -- binary ops --
      case OP_ADD:      BINARY_OP(NUMBER_VAL, +); break;
      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;

      // -- unary ops --
      case OP_NEGATE:
        if (!IS_NUMBER(peek(0))) {
          runtime_error("Operand must be a number.");
          return INTERPRET_RUNTIME_ERR;
        }

        // same thing as the following, just mutates in-place
        //
        //     push(NUMBER_VAL(-AS_NUMBER(pop())))
        //
        (vm.stack_top - 1)->as.number *= -1;
        break;

      case OP_RETURN: {
        print_value(pop());
        printf("\n");

        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONST
#undef BINARY_OP
}

InterpretResult interpret(const char* source) {
  Chunk chunk;

  init_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code;
  InterpretResult res = run();

  free_chunk(&chunk);
  return res;
}

void push(Value val) {
  *vm.stack_top = val;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}

static Value peek(size_t distance) {
  return vm.stack_top[-1 - distance];
}
