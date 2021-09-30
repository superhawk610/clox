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

static ObjString* allocate_string(char* chars, size_t len) {
  ObjString* str = ALLOCATE_OBJ(ObjString, OBJ_STRING);
  str->len = len;
  str->chars = chars;

  return str;
}

ObjString* take_string(char* chars, size_t len) {
  return allocate_string(chars, len);
}

ObjString* copy_string(const char* chars, size_t len) {
  char* heap_chars = ALLOCATE(char, len + 1);
  memcpy(heap_chars, chars, len);
  heap_chars[len] = '\0'; // good ol' null-terminated strings

  return allocate_string(heap_chars, len);
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
