#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#define GLOBALS_LRU_CACHE_CAP 25

VM vm; // global singleton, since we don't support parallel VMs

static void reset_stack() {
  vm.stack_top = vm.stack;
}

void init_vm() {           // initialize the VM:
  reset_stack();           // 1. reset the stack
  vm.objects = NULL;       // 2. initialize object storage (for GC)
  init_table(&vm.globals); // 3. initialize global variable storage
  init_table(&vm.strings); // 4. initialize interned string storage
}

void free_vm() {
  free_table(&vm.globals);
  free_table(&vm.strings);
  free_objects();
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

static bool is_falsey(Value val) {
  return IS_NIL(val) || (IS_BOOL(val) && !AS_BOOL(val));
}

static void concatenate_strings() {
  ObjString* b = AS_STRING(pop());
  ObjString* a = AS_STRING(pop());

  size_t len = a->len + b->len;
  char* chars = ALLOCATE(char, len + 1);
  memcpy(chars,          a->chars, a->len);
  memcpy(chars + a->len, b->chars, b->len);
  chars[len] = '\0';

  ObjString* res = take_string(chars, len);
  push(OBJ_VAL((Obj*) res));
}

static InterpretResult run() {
#define READ_BYTE() (*vm.ip++)
#define READ_CONST() (vm.chunk->constants.values[READ_BYTE()])
#define READ_CONST_LONG() (vm.chunk->constants.values[(READ_BYTE() << 8) | READ_BYTE()])
#define READ_SHORT() (vm.ip += 2, (uint16_t)((vm.ip[-2] << 8) | vm.ip[-1]))
#define READ_STRING() AS_STRING(READ_CONST())
#define READ_STRING_LONG() AS_STRING(READ_CONST_LONG())

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
      case OP_CONST_LONG: {
        Value constant = READ_CONST_LONG();
        push(constant);

        break;
      }

      // -- intrinsic constants --
      case OP_NIL:   push(NIL_VAL); break;
      case OP_TRUE:  push(BOOL_VAL(true)); break;
      case OP_FALSE: push(BOOL_VAL(false)); break;

      // -- misc. --
      case OP_POP: pop(); break;

      case OP_GET_LOCAL: {
        uint8_t slot = READ_BYTE();
        push(vm.stack[slot]);
        break;
      }

      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        vm.stack[slot] = peek(0);
        break;
      }

      case OP_DEF_GLOBAL: {
        ObjString* name = READ_STRING();
        table_set(&vm.globals, name, peek(0));
        pop();
        break;
      }
      case OP_DEF_GLOBAL_LONG: {
        ObjString* name = READ_STRING_LONG();
        table_set(&vm.globals, name, peek(0));
        pop();
        break;
      }

      case OP_GET_GLOBAL: {
        ObjString* name = READ_STRING();
        Value val;

        if (!table_get(&vm.globals, name, &val)) {
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERR;
        }

        push(val);
        break;
      }
      case OP_GET_GLOBAL_LONG: {
        ObjString* name = READ_STRING_LONG();
        Value val;

        if (!table_get(&vm.globals, name, &val)) {
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERR;
        }

        push(val);
        break;
      }

      case OP_SET_GLOBAL: {
        ObjString* name = READ_STRING();

        if (table_set(&vm.globals, name, peek(0))) {              // if this was the first time we've
          table_delete(&vm.globals, name);                        // seen this variable, it's set-
          runtime_error("Undefined variable '%s'.", name->chars); // before-define, which isn't allowed
          return INTERPRET_RUNTIME_ERR;
        }

        break;
      }
      case OP_SET_GLOBAL_LONG: {
        ObjString* name = READ_STRING_LONG();

        if (table_set(&vm.globals, name, peek(0))) {
          table_delete(&vm.globals, name);
          runtime_error("Undefined variable '%s'.", name->chars);
          return INTERPRET_RUNTIME_ERR;
        }

        break;
      }

      // -- binary ops --
      case OP_ADD: { // the + operator is special because it can
                     // operate on both numbers and strings
        if (IS_STRING(peek(0)) && IS_STRING(peek(1))) {
          concatenate_strings();
        } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))) {
          double b = AS_NUMBER(pop());
          double a = AS_NUMBER(pop());
          push(NUMBER_VAL(a + b));
        } else {
          runtime_error("Operands must be two strings or two numbers.");
          return INTERPRET_RUNTIME_ERR;
        }

        break;
      }

      case OP_SUBTRACT: BINARY_OP(NUMBER_VAL, -); break;
      case OP_MULTIPLY: BINARY_OP(NUMBER_VAL, *); break;
      case OP_DIVIDE:   BINARY_OP(NUMBER_VAL, /); break;

      case OP_GREATER:  BINARY_OP(BOOL_VAL, >); break;
      case OP_LESS:     BINARY_OP(BOOL_VAL, <); break;

      case OP_EQUAL: {
        Value b = pop();
        Value a = pop();
        push(BOOL_VAL(values_equal(a, b)));
        break;
      }

      // -- unary ops --
      case OP_NOT: push(BOOL_VAL(is_falsey(pop()))); break;
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

      // -- statements --
      case OP_PRINT: {
        print_value(pop());
        printf("\n");
        break;
      }

      // -- control flow --
      case OP_JUMP: {
        uint16_t offset = READ_SHORT();
        vm.ip += offset;
        break;
      }

      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (is_falsey(peek(0))) vm.ip += offset;
        break;
      }

      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        vm.ip -= offset;
        break;
      }

      case OP_RETURN:
        // exit the interpreter
        return INTERPRET_OK;
    }
  }

#undef READ_BYTE
#undef READ_CONST
#undef READ_CONST_LONG
#undef READ_SHORT
#undef READ_STRING
#undef READ_STRING_LONG
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
