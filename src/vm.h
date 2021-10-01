#ifndef __CLOX_VM_H__
#define __CLOX_VM_H__

#include "chunk.h"
#include "lru.h"
#include "table.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
  Chunk* chunk;
  uint8_t* ip; // instruction pointer (aka program counter)

  Value  stack[STACK_MAX]; // value stack manipulator (used to track
  Value* stack_top;        // temporary values during execution)

  LRUCache globals_cache; // LRU cache for hot global variables
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
