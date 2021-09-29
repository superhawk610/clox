#include <assert.h>
#include <stdio.h>
#include "rle_array.h"

void init_rle_array(RLEArray* arr) {
  arr->data = NULL;
  arr->cap = 0;
  arr->len = 0;
}

void init_rle_tuple(RLETuple* tup, int val) {
  tup->count = 1;
  tup->value = val;
}

void push_rle_array(RLEArray* arr, int val) {
#define PEEK_R(arr) \
  (arr->len == 0 ? NULL : &arr->data[arr->len - 1])

  RLETuple* last = PEEK_R(arr);
  if (last != NULL && last->value == val) {
    last->count++;
    return;
  }

  if (arr->len == arr->cap) {
    size_t old_cap = arr->cap;
    arr->cap = GROW_CAPACITY(old_cap);
    arr->data = GROW_ARRAY(RLETuple, arr->data, old_cap, arr->cap);
  }

  init_rle_tuple(&arr->data[arr->len], val);
  arr->len++;

#undef PEEK_R
}

int get_nth_rle_array(RLEArray* arr, size_t n) {
  RLETuple* tup;

  for (size_t offset = 0; offset < arr->len; offset++) {
    tup = &arr->data[offset];

    if (n <= tup->count) return tup->value;
    n -= tup->count;
  }

  assert(false && "unreachable (who needs bounds checks)");
}

void free_rle_array(RLEArray* arr) {
  FREE_ARRAY(RLETuple, arr->data, arr->cap);
  init_rle_array(arr);
}
