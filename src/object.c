#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
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

ObjString* take_string(char* chars, size_t len) {
  uint32_t hash = hash_string(chars, len);
  return allocate_string(chars, len, hash);
}

ObjString* copy_string(const char* chars, size_t len) {
  uint32_t hash = hash_string(chars, len);
  char* heap_chars = ALLOCATE(char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0'; // good ol' null-terminated strings

  return allocate_string(heap_chars, len, hash);
}

void print_object(Value val) {
  switch (OBJ_TYPE(val)) {
    case OBJ_STRING:
      printf("%s", AS_CSTRING(val));
      break;
  }
}

// ---

#undef ALLOCATE_OBJ
