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
