#include <stdio.h>
#include <string.h>
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

ObjFunction* new_function() {
  ObjFunction* func = ALLOCATE_OBJ(ObjFunction, OBJ_FUNCTION);

  func->arity = 0;
  func->name = NULL;
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
    printf("<script>");
    return;
  }

  printf("<fn %s>", func->name->chars);
}

void print_object(Value val) {
  switch (OBJ_TYPE(val)) {
    case OBJ_FUNCTION:
      print_function(AS_FUNCTION(val));
      break;
    case OBJ_NATIVE:
      printf("<native fn>");
      break;
    case OBJ_STRING:
      printf("%s", AS_CSTRING(val));
      break;
  }
}

// ---

#undef ALLOCATE_OBJ
