#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "value.h"

void init_value_array(ValueArray* arr) {
  arr->values = NULL;
  arr->cap = 0;
  arr->len = 0;
}

void free_value_array(ValueArray* arr) {
  FREE_ARRAY(Value, arr->values, arr->cap);
  init_value_array(arr);
}

void write_value_array(ValueArray* arr, Value val) {
  if (arr->len == arr->cap) {
    size_t old_cap = arr->cap;
    arr->cap = GROW_CAPACITY(old_cap);
    arr->values = GROW_ARRAY(Value, arr->values, old_cap, arr->cap);
  }

  arr->values[arr->len] = val;
  arr->len++;
}

bool values_equal(Value a, Value b) {
  if (a.type != b.type) return false; // equality will always be false across types

  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);

    case VAL_OBJ: {
      ObjString* a_str = AS_STRING(a);
      ObjString* b_str = AS_STRING(b);

      // equal strings must be the same length and contain
      // the same sequence of characters
      return a_str->len == b_str->len &&
             memcmp(a_str->chars, b_str->chars, a_str->len) == 0;
    }

    default: return false; // unreachable
  }
}

void print_value(Value val) {
  switch (val.type) {
    case VAL_BOOL:
      printf("%s%s%s", ANSI_Cyan,
                       AS_BOOL(val) ? "true" : "false",
                       ANSI_Reset);
      break;
    case VAL_NIL:    printf(ANSI_Dim "nil" ANSI_Reset); break;
    case VAL_NUMBER: printf("%g", AS_NUMBER(val)); break;
    case VAL_OBJ:    print_object(val); break;
  }
}
