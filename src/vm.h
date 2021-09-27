#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* ip; // instruction pointer (aka program counter)

  Value  stack[STACK_MAX]; // value stack manipulator (used to track
  Value* stack_top;        // temporary values during execution)
} VM;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERR,
  INTERPRET_RUNTIME_ERR,
} InterpretResult;

void init_vm();

void free_vm();

InterpretResult interpret(const char* source);

void push(Value val);

Value pop();

#endif // __CLOX_VM_H__
