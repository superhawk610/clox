#ifndef __CLOX_VALUE_H__
#define __CLOX_VALUE_H__

#include "common.h"

typedef struct Obj Obj;
typedef struct ObjString ObjString;

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJ,
} ValueType;

/**
 * Values are stored as tagged unions, where the first byte
 * determines what type of value is contained, and the remaining
 * byte(s) store the value itself.
 *
 * [ ][ ][ ][ ] [ ][ ][ ][ ] [ ][ ][ ][ ][ ][ ][ ][ ]
 * |---type---| |-padding--| |----------as----------|
 *                   boolean |-|
 *                    number |----------------------|
 *                       obj |----------------------|
 */
typedef struct {
  ValueType type;
  union {
    bool boolean;
    double number;
    Obj* obj;
  } as;
} Value;

#define IS_BOOL(val)    ((val).type == VAL_BOOL)
#define IS_NIL(val)     ((val).type == VAL_NIL)
#define IS_NUMBER(val)  ((val).type == VAL_NUMBER)
#define IS_OBJ(val)     ((val).type == VAL_OBJ)

#define AS_BOOL(val)    ((val).as.boolean)
#define AS_NUMBER(val)  ((val).as.number)
#define AS_OBJ(val)     ((val).as.obj)

#define BOOL_VAL(val)   ((Value) {VAL_BOOL,   {.boolean = val}})
#define NIL_VAL         ((Value) {VAL_NIL,    {.number  = 0  }})
#define NUMBER_VAL(val) ((Value) {VAL_NUMBER, {.number  = val}})
#define OBJ_VAL(ptr)    ((Value) {VAL_OBJ,    {.obj     = ptr}})

typedef struct {
  // dynamic array containing zero or more values
  Value* values;

  size_t len; // number of values in array
  size_t cap; // available spaces for values in array
} ValueArray;

void init_value_array(ValueArray* arr);

void free_value_array(ValueArray* arr);

void write_value_array(ValueArray* arr, Value val);

bool values_equal(Value a, Value b);

void print_value(Value val);

#endif // __CLOX_VALUE_H__
