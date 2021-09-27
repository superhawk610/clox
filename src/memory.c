#include <stdlib.h>
#include "memory.h"

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
