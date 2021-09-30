#include <stdio.h>
#include "memory.h"
#include "value.h"

void init_value_array(ValueArray* arr) {
  arr->values = NULL;
  arr->cap = 0;
  arr->len = 0;
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

void free_value_array(ValueArray* arr) {
  FREE_ARRAY(Value, arr->values, arr->cap);
  init_value_array(arr);
}

void print_value(Value val) {
  printf("%g", AS_NUMBER(val));
}
