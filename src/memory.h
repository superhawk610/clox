#ifndef __CLOX_MEMORY_H__
#define __CLOX_MEMORY_H__

#include "common.h"
#include "object.h"

#define ALLOCATE(type, count) \
  ((type*) reallocate(NULL, 0, sizeof(type) * (count)))

// start capacity at 8 to avoid initial churn when
// allocating small arrays
#define GROW_CAPACITY(cap) \
  ((cap) < 8 ? 8 : (cap) * 2)

#define GROW_ARRAY(type, ptr, old_len, new_len) \
  ((type*) reallocate(ptr, sizeof(type) * (old_len), sizeof(type) * (new_len)))

#define FREE_ARRAY(type, ptr, old_len) \
  reallocate(ptr, sizeof(type) * (old_len), 0)

#define FREE(type, ptr) \
  reallocate(ptr, sizeof(type), 0)

void* reallocate(void* ptr, size_t old_len, size_t new_len);

void free_objects();

#endif // __CLOX_MEMORY_H__
