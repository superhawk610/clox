#include <stdio.h>
#include <string.h>
#include "logger.h"
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

void value_array_push(ValueArray* arr, Value val) {
  if (arr->len == arr->cap) {
    size_t old_cap = arr->cap;
    arr->cap = GROW_CAPACITY(old_cap);
    arr->values = GROW_ARRAY(Value, arr->values, old_cap, arr->cap);
  }

  arr->values[arr->len] = val;
  arr->len++;
}

/** @return true if the value was contained in the array */
bool value_array_find_index(ValueArray* arr, Value val, uint16_t* out) {
  for (size_t i = 0; i < arr->len; i++) {
    if (values_equal(val, arr->values[i])) {
      *out = i;
      return true;
    }
  }

  return false;
}

bool values_equal(Value a, Value b) {
  if (a.type != b.type) return false; // equality will always be false across types

  switch (a.type) {
    case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
    case VAL_NIL:    return true;
    case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);

                     // duplicate interned strings will point at the same memory
    case VAL_OBJ:    return AS_OBJ(a) == AS_OBJ(b);

    default: return false; // unreachable
  }
}

void print_value(Value val) {
  switch (val.type) {
    case VAL_BOOL:
      out_printf("%s%s%s", ANSI_Cyan,
                           AS_BOOL(val) ? "true" : "false",
                           ANSI_Reset);
      break;
    case VAL_NIL:    out_printf(ANSI_Dim "nil" ANSI_Reset); break;
    case VAL_NUMBER: out_printf("%g", AS_NUMBER(val)); break;
    case VAL_OBJ:    print_object(val); break;
  }
}
