#include <stdio.h>
#include <string.h>
#include "logger.h"
#include "memory.h"
#include "object.h"
#include "table.h"
#include "vm.h"

#define ALLOCATE_OBJ(type, obj_type) \
  ((type*) allocate_object(sizeof(type), obj_type))

static Obj* allocate_object(size_t size, ObjType type) {
  Obj* obj = (Obj*) reallocate(NULL, 0, size);
  obj->type = type;

  // prepend to `vm.objects` linked list
  obj->next = vm.objects;
  vm.objects = obj;

  return obj;
}

static ObjString* allocate_string(char* chars, size_t len, uint32_t hash) {
  ObjString* str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  str->len = len;
  str->chars = chars;
  str->hash = hash;

  // intern the string
  table_set(&vm.strings, str, NIL_VAL);

  return str;
}

static uint32_t hash_string(const char* chars, size_t len) {
  uint32_t hash = 2166136261u; // base for FNV-1a

  for (size_t i = 0; i < len; i++) {
    hash ^= (uint8_t) chars[i];
    hash *= 16777619;
  }

  return hash;
}

ObjClosure* new_closure(ObjFunction* func) {
  ObjUpvalue** uvs = ALLOCATE(ObjUpvalue*, func->upvalue_count);
  for (int i = 0; i < func->upvalue_count; i++) {
    uvs[i] = NULL;
  }

  ObjClosure* closure = ALLOCATE_OBJ(ObjClosure, OBJ_CLOSURE);
  closure->function = func;
  closure->upvalues = uvs;
  closure->upvalue_count = func->upvalue_count;
  return closure;
}

ObjFunction* new_function() {
  ObjFunction* func = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  func->name = NULL;
  func->arity = 0;
  func->upvalue_count = 0;
  init_chunk(&func->chunk); // heh, func chunk

  return func;
}

ObjNative* new_native(NativeFn func) {
  ObjNative* native = ALLOCATE_OBJ(ObjNative, OBJ_NATIVE);
  native->function = func;
  return native;
}

ObjString* take_string(char* chars, size_t len) {
  uint32_t hash = hash_string(chars, len);

  ObjString* interned = table_find_string(&vm.strings, chars, len, hash);
  if (interned != NULL) {
    FREE_ARRAY(char, chars, len + 1); // if the we're using the interned copy, it's
    return interned;                  // up to us to free the memory used by `chars`
  }                                   // (normally, the table would take ownership)

  return allocate_string(chars, len, hash);
}

ObjString* copy_string(const char* chars, size_t len) {
  uint32_t hash = hash_string(chars, len);

  // if the string's already been interned, return that
  ObjString* interned = table_find_string(&vm.strings, chars, len, hash);
  if (interned != NULL) return interned;

  char* heap_chars = ALLOCATE(char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0'; // good ol' null-terminated strings

  return allocate_string(heap_chars, len, hash);
}

static void print_function(ObjFunction* func) {
  if (func->name == NULL) { // the top-level "function" has no name
    out_printf("<script>");
    return;
  }

  out_printf("<fn %s>", func->name->chars);
}

ObjUpvalue* new_upvalue(Value* slot) {
  ObjUpvalue* uv = ALLOCATE_OBJ(ObjUpvalue, OBJ_UPVALUE);
  uv->closed = NIL_VAL;
  uv->location = slot;
  uv->next = NULL;
  return uv;
}

void print_object(Value val) {
  switch (OBJ_TYPE(val)) {
    case OBJ_CLOSURE:
      print_function(AS_CLOSURE(val)->function);
      break;
    case OBJ_FUNCTION:
      print_function(AS_FUNCTION(val));
      break;
    case OBJ_NATIVE:
      out_printf("<native fn>");
      break;
    case OBJ_STRING:
      out_printf("%s", AS_CSTRING(val));
      break;
    case OBJ_UPVALUE: // upvalues are runtime internals, so user code
                      // shouldn't actually ever hit this branch
      printf("upvalue");
      break;
  }
}

// ---

#undef ALLOCATE_OBJ
