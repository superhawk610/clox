#ifndef __CLOX_RLE_ARRAY_H__
#define __CLOX_RLE_ARRAY_H__

#include "memory.h"

typedef struct {
  size_t count; // the number of consecutives values in this run
  int value; // the value itself
} RLETuple;

/**
 * Run-length encoded (RLE) array of positive integers.
 *
 * Elements are compressed by denoting _runs_ of data as tuples, each
 * describing the value of the element and the number of consecutive
 * values with that same value. This scales poorly for highly-diverse
 * data, but compresses very well for data with frequent runs.
 */
typedef struct {
  RLETuple* data; // pointer to the underlying memory

  size_t len; // number of run tuples in the array
  size_t cap; // available slots for run tuples in the array
} RLEArray;

void init_rle_array(RLEArray* arr);

void push_rle_array(RLEArray* arr, int val);

/**
 * Get the n-th element of a RLE array.
 *
 * nth([(2x 1), (3x 2), (1x 3)], 0) // 1
 * nth(",                        1) // 1
 * nth(",                        2) // 2
 * ...
 * nth(",                        5) // 3
 */
int get_nth_rle_array(RLEArray* arr, size_t n);

void free_rle_array(RLEArray* arr);

#endif // __CLOX_RLE_ARRAY_H__
