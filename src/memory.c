#include <stdlib.h>
#include "memory.h"
#include "object.h"
#include "vm.h"

/**
 * TODO: hard-mode challenge
 *
 * Instead of using the `stdlib` memory allocator, try implementing your
 * own. You're allowed a single call to `malloc` during program init to
 * grab an initial block of memory, then expected to parcel out via your
 * own implementation of `realloc`.
 *
 * ref: https://code.woboq.org/userspace/glibc/malloc/malloc.c.html#1323
 */

void* reallocate(void* ptr, size_t old_len, size_t new_len) {
  // setting length to 0 is equivalent to deallocating
  if (new_len == 0) {
    free(ptr);
    return NULL;
  }

  void* res = realloc(ptr, new_len);
  if (res == NULL) exit(1); // nothing much else to do
  return res;
}

static void free_object(Obj* obj) {
  switch (obj->type) {
    case OBJ_CLOSURE: {
      ObjClosure* closure = (ObjClosure*) obj;
      FREE_ARRAY(ObjUpvalue*, closure->upvalues, closure->upvalue_count);
      FREE(ObjClosure, obj);
      break;
    }
    case OBJ_FUNCTION: {
      ObjFunction* func = (ObjFunction*) obj;
      free_chunk(&func->chunk);
      FREE(ObjFunction, obj);
      break;
    }
    case OBJ_NATIVE:
      FREE(ObjNative, obj);
      break;
    case OBJ_STRING: {
      ObjString* str = (ObjString*) obj;
      FREE_ARRAY(char, str->chars, str->len + 1);
      FREE(ObjString, obj);
      break;
    }
    case OBJ_UPVALUE:
      FREE(ObjUpvalue, obj);
      break;
  }
}

void free_objects() {
  Obj* obj = vm.objects;

  while (obj != NULL) {
    Obj* next = obj->next;
    free_object(obj);
    obj = next;
  }
}
