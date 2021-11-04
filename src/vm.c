#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "common.h"
#include "compiler.h"
#include "debug.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

VM vm; // global singleton, since we don't support parallel VMs

// -- native functions

static Value clock_native(uint8_t argc, Value* argv) {
  return NUMBER_VAL((double) clock() / CLOCKS_PER_SEC);
}

static void define_native(const char* name, NativeFn func) {
  push(OBJ_VAL((Obj*) copy_string(name, (size_t) strlen(name))));
  push(OBJ_VAL((Obj*) new_native(func)));
  table_set(&vm.globals, AS_STRING(vm.stack[0]), vm.stack[1]);
  pop(); // push then pop the values onto the stack to include
  pop(); // them in garbage collection
}

// --

static void reset_stack() {
  vm.stack_top = vm.stack;
  vm.frame_count = 0;
}

void init_vm() {           // initialize the VM:
  reset_stack();           // 1. reset the stack
  vm.objects = NULL;       // 2. initialize object storage (for GC)
  init_table(&vm.globals); // 3. initialize global variable storage
  init_table(&vm.strings); // 4. initialize interned string storage

  // 5. define native functions
  define_native("clock", clock_native);
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

  // print stacktrace (in reverse order of execution, most recent first)
  for (int i = vm.frame_count - 1; i >= 0; i--) {
    StackFrame* frame = &vm.frames[i];
    ObjFunction* func = frame->closure->function;
    size_t instruction = frame->ip - func->chunk.code - 1;

    fprintf(stderr, "[line %d] in ", get_nth_rle_array(&func->chunk.lines, instruction));
    if (func->name == NULL) {
      fprintf(stderr, "script\n");
    } else {
      fprintf(stderr, "%s()\n", func->name->chars);
    }
  }

  reset_stack();
}

// When executing a function call, the function itself will be pushed
// onto the stack, followed by the arguments to the call. Then, a new
// stack frame rooted at the function object will be created, and the
// VM's instruction pointer will be updated to point there, where
// execution will be resumed.
//
//     func(a, b, c);
//
//               ┌────────────────────┐
//     [-] [...] | [func] [a] [b] [c] |
//               └───┬────────────────┘
//                   └─ stack frame
//
static bool call(ObjClosure* closure, uint8_t argc) {
  if (argc != closure->function->arity) {
    runtime_error("Expected %d arguments but got %d.", closure->function->arity, argc);
    return false;
  }

  if (vm.frame_count == FRAMES_MAX) {
    runtime_error("Stack overflow."); // hey look, it's the thing!!
    return false;
  }

  StackFrame* frame = &vm.frames[vm.frame_count++];

  frame->closure = closure;
  frame->ip = closure->function->chunk.code;
  frame->slots = vm.stack_top - argc - 1;

  return true;
}

static bool call_value(Value callee, uint8_t argc) {
  if (IS_OBJ(callee)) {
    switch (OBJ_TYPE(callee)) {
      case OBJ_CLOSURE:
        return call(AS_CLOSURE(callee), argc);
      case OBJ_NATIVE: {                                  // to call a native function, invoke
        NativeFn native = AS_NATIVE(callee);              // the C function pointer, store its
        Value result = native(argc, vm.stack_top - argc); // return value, then push it on the
        vm.stack_top -= argc + 1;                         // stack and resume execution
        push(result);
        return true;
      }
      default:
        break; // object must not have been callable
    }
  }

  runtime_error("Can only call functions and closures.");
  return false;
}

static ObjUpvalue* capture_upvalue(Value* local) {
  ObjUpvalue* created_uv = new_upvalue(local);
  return created_uv;
}

static inline bool is_falsey(Value val) {
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

// TODO: Store the instruction pointer in a register and benchmark.
// TODO: Add arity checking for native functions.
// TODO: Allow native functions to signal runtime errors.
// TODO: Implement additional native functions.
// (see https://craftinginterpreters.com/calls-and-functions.html#challenges)

static InterpretResult run() {
  StackFrame* frame = &vm.frames[vm.frame_count - 1];

#define READ_BYTE() (*frame->ip++)
#define READ_CONST() (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_CONST_LONG() (frame->closure->function->chunk.constants.values[(READ_BYTE() << 8) | READ_BYTE()])
#define READ_SHORT() (frame->ip += 2, (uint16_t)((frame->ip[-2] << 8) | frame->ip[-1]))
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
    disasm_instruction(&frame->closure->function->chunk,
                      (size_t) (frame->ip - frame->closure->function->chunk.code));
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
        push(frame->slots[slot]);
        break;
      }

      case OP_SET_LOCAL: {
        uint8_t slot = READ_BYTE();
        frame->slots[slot] = peek(0); // assignment is an expression, so the value
        break;                        // remains on the stack (isn't popped)
      }

      case OP_GET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        push(*frame->closure->upvalues[slot]->location);
        break;
      }

      case OP_SET_UPVALUE: {
        uint8_t slot = READ_BYTE();
        *frame->closure->upvalues[slot]->location = peek(0);
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
        frame->ip += offset;
        break;
      }

      case OP_JUMP_IF_FALSE: {
        uint16_t offset = READ_SHORT();
        if (is_falsey(peek(0))) frame->ip += offset;
        break;
      }

      case OP_LOOP: {
        uint16_t offset = READ_SHORT();
        frame->ip -= offset;
        break;
      }

      case OP_CALL: {
        uint8_t argc = READ_BYTE();
        if (!call_value(peek(argc), argc)) {
          return INTERPRET_RUNTIME_ERR;
        }

        // if the function call succeeds, remove its stack frame
        frame = &vm.frames[vm.frame_count - 1];

        break;
      }

      case OP_CLOSURE: {
        ObjFunction* func = AS_FUNCTION(READ_CONST());
        ObjClosure* closure = new_closure(func);
        push(OBJ_VAL((Obj*) closure));

        for (int i = 0; i < closure->upvalue_count; i++) {
          bool is_local = READ_BYTE() == 1 ? true : false;
          uint8_t index = READ_BYTE();

          // if we're capturing a local upvalue, it's our job to grab its value
          if (is_local) {
            closure->upvalues[i] = capture_upvalue(frame->slots + index);
          // otherwise, an enclosing scope has already grabbed the value, so just
          // point to that upvalue (which may in turn point to another)
          } else {
            closure->upvalues[i] = frame->closure->upvalues[index];
          }
        }

        break;
      }

      case OP_RETURN: {
        Value result = pop();
        vm.frame_count--;

        if (vm.frame_count == 0) { // we've finish executing the top-level code
          pop();                   // so clean up the stack and exit
          return INTERPRET_OK;
        }

        // otherwise, we're returning from a function call, so shift the stack
        // back to where it was before the function call, then push the result
        // of the call onto the stack, and shift to the most recent stack frame
        vm.stack_top = frame->slots;
        push(result);
        frame = &vm.frames[vm.frame_count - 1];
        break;
      }
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
  ObjFunction* func = compile(source);
  if (func == NULL) return INTERPRET_COMPILE_ERR;

  // the first slot in the stack contains the top-level function
  // (which is really just a function with no arguments, right?)
  ObjClosure* closure = new_closure(func);
  push(OBJ_VAL((Obj*) closure));
  call(closure, 0);

  return run();
}
