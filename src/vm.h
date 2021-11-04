#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"
#include "table.h"
#include "value.h"
#include "object.h"

#define FRAMES_MAX 64
#define STACK_MAX  (FRAMES_MAX * UINT8_COUNT)

/**
 * A stack frame that tracks the relative "call frame" (or window)
 * of a function invocation. The instruction pointer refers to the
 * caller's return pointer, and will be used to resume execution
 * once a function call completes.
 *
 * The `slots` array can be used as a drop-in replacement for the
 * value stack, as it points at some offset inside the VM's stack.
 *
 * For example, take this code:
 *
 *     fun f() {
 *       var x = 1;
 *       var y = 2;
 *     }
 *
 *     var a = 1;
 *     f();
 *
 *     var b = 2;
 *     f();
 *
 * Here's the stack after the first call to `f()`:
 *
 *         ┌─────────┐
 *     [a] | [x] [y] |
 *         └─ 0 ┬ 1 ─┘
 *              └─ stack frame
 *
 * In the bytecode, x and y aren't stored as absolute offsets into
 * the VM's stack, but are instead stored as relative offsets into
 * their enclosing function's call frame. In this example, x would
 * be stored as `0` and y as `1`.
 *
 * Here's the stack after the second call to `f()`:
 *
 *             ┌─────────┐
 *     [a] [b] | [x] [y] |
 *             └─ 0 ─ 1 ─┘
 *
 */
typedef struct {
  ObjClosure* closure;
  uint8_t* ip;
  Value* slots;
} StackFrame;

typedef struct {
  Chunk* chunk;
  uint8_t* ip; // instruction pointer (aka program counter)

  StackFrame frames[FRAMES_MAX];
  int        frame_count;

  Value  stack[STACK_MAX]; // value stack manipulator (used to track
  Value* stack_top;        // temporary values during execution)

  // TODO: How could global variable storage be improved?
  //
  // Currently, global variables are stored in a string->value
  // table. In the bytecode, each "global variable" is really
  // just a reference to a constant in the current chunk, which
  // in turn is used as a key to get/set the global's value.
  //
  // Instead of using a string-keyed table, globals could be
  // stored in a dynamically-sized array, keyed by index.

  Table    globals; // storage for global variables at runtime
  Table    strings; // container for interned strings
  Obj*     objects; // linked-list for naive garbage collection
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERR,
  INTERPRET_RUNTIME_ERR,
} InterpretResult;

extern VM vm;

void init_vm();

void free_vm();

InterpretResult interpret(const char* source);

void push(Value val);

Value pop();

#endif // __CLOX_VM_H__
