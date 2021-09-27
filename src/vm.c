#include <stdio.h>
#include "common.h"
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

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONST() (vm.chunk->constants.values[READ_BYTE()])

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
      case OP_RETURN: {
        print_value(pop());
        printf("\n");

        return INTERPRET_OK;
      }
    }
  }

#undef READ_BYTE
#undef READ_CONST
}

InterpretResult interpret(Chunk* chunk) {
  vm.chunk = chunk;
  vm.ip = vm.chunk->code;

  return run();
}

void push(Value val) {
  *vm.stack_top = val;
  vm.stack_top++;
}

Value pop() {
  vm.stack_top--;
  return *vm.stack_top;
}
