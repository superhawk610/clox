#ifndef __CLOX_OBJECT_H__
#define __CLOX_OBJECT_H__

#include "common.h"
#include "chunk.h"
#include "value.h"

#define OBJ_TYPE(val)     (AS_OBJ(val)->type)

#define IS_CLOSURE(val)   is_obj_type(val, OBJ_CLOSURE)
#define IS_FUNCTION(val)  is_obj_type(val, OBJ_FUNCTION)
#define IS_NATIVE(val)    is_obj_type(val, OBJ_NATIVE)
#define IS_STRING(val)    is_obj_type(val, OBJ_STRING)

#define AS_CLOSURE(val)   ((ObjClosure*) AS_OBJ(val))
#define AS_FUNCTION(val)  ((ObjFunction*) AS_OBJ(val))
#define AS_NATIVE(val)    (((ObjNative*) AS_OBJ(val))->function)
#define AS_STRING(val)    ((ObjString*) AS_OBJ(val))
#define AS_CSTRING(val)   (((ObjString*) AS_OBJ(val))->chars)

typedef enum {
  OBJ_CLOSURE,
  OBJ_FUNCTION,
  OBJ_NATIVE,
  OBJ_STRING,
  OBJ_UPVALUE,
} ObjType;

/**
 * Obj serves as the "base class" for all object "subclasses",
 * which in turn "inherit" from it. Since C doesn't actually
 * support direct OOP, this is simulated by using a common
 * struct layout for all "subclasses", with a type header.
 *
 *     [ ][ ][ ][ ] [ ][ ][ ][ ]...
 *     |-- type --| |-------------->
 *        header     subclass fields
 *
 * Additionally, C guarantees that the first field of a struct
 * will always be the first in memory, so it's safe to convert
 * back and forth between a pointer to a struct and a pointer
 * to its first field. That means, if we have a pointer to an Obj
 *
 *     Obj* obj;
 *
 * we can treat it as an ObjString (assuming it actually is)
 *
 *     ObjString* obj_str = (ObjString*) obj;
 */
struct Obj {
  ObjType type;
  struct Obj* next; // track usage for naive garbage collection
};

typedef struct {
  Obj obj;
  Chunk chunk;

  ObjString* name;   // identifying info about the function
  int arity;         // (name, arity)

  int upvalue_count; // how many upvalues are captured
} ObjFunction;

typedef Value (*NativeFn)(uint8_t argc, Value* argv);

typedef struct {
  Obj obj;
  NativeFn function;
} ObjNative;

struct ObjString {
  Obj obj;
  size_t len;
  char* chars;

  uint32_t hash; // eagerly-computed hash
};

typedef struct ObjUpvalue {
  Obj obj;
  Value* location; // reference to the captured variable; note that
                   // this is a Value*, not just a Value, pointing
                   // to the upstream variable so that mutations made
                   // via this upvalue are reflected in the original
                   // scope as well

  Value closed; // once an upvalue is "closed", it stores the value of
                // the closed-over variable in this field (on the heap)

  struct ObjUpvalue* next;
} ObjUpvalue;

// Closures point to a function to invoke, along with zero
// or more captured variables from the defining scope.
typedef struct {
  Obj obj;
  ObjFunction* function;
  ObjUpvalue** upvalues; // dynamic array for captured upvalues
  int upvalue_count;
} ObjClosure;

ObjClosure* new_closure(ObjFunction* func);

ObjFunction* new_function();

ObjNative* new_native(NativeFn func);

ObjString* take_string(char* chars, size_t len);

ObjString* copy_string(const char* chars, size_t len);

ObjUpvalue* new_upvalue(Value* slot);

void print_object(Value val);

static inline bool is_obj_type(Value val, ObjType type) {
  return IS_OBJ(val) && OBJ_TYPE(val) == type;
}

#endif // __CLOX_OBJECT_H__
