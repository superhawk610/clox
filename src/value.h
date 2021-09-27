#ifndef __CLOX_VALUE_H__
#define __CLOX_VALUE_H__

#include "common.h"

typedef double Value;

typedef struct {
  // dynamic array containing zero or more values
  Value* values;

  size_t len; // number of values in array
  size_t cap; // available spaces for values in array
} ValueArray;

void init_value_array(ValueArray* arr);

void write_value_array(ValueArray* arr, Value val);

void free_value_array(ValueArray* arr);

void print_value(Value val);

#endif // __CLOX_VALUE_H__
