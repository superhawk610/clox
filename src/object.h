#ifndef __CLOX_OBJECT_H__
#define __CLOX_OBJECT_H__

#include "common.h"
#include "value.h"

#define OBJ_TYPE(val)   (AS_OBJ(val)->type)

#define IS_STRING(val)  is_obj_type(val, OBJ_STRING)

#define AS_STRING(val)  ((ObjString*) AS_OBJ(val))
#define AS_CSTRING(val) (((ObjString*) AS_OBJ(val))->chars)

typedef enum {
  OBJ_STRING,
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

struct ObjString {
  Obj obj;
  size_t len;
  char* chars;
};

ObjString* take_string(char* chars, size_t len);

ObjString* copy_string(const char* chars, size_t len);

void print_object(Value val);

static inline bool is_obj_type(Value val, ObjType type) {
  return IS_OBJ(val) && OBJ_TYPE(val) == type;
}

#endif // __CLOX_OBJECT_H__
