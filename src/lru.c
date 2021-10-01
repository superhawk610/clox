#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include "lru.h"
#include "memory.h"

void init_cache(LRUCache* cache, size_t cap) {
  if (cap == 0) {
    fprintf(stderr, "cannot initialize cache with 0 capacity");
    exit(EX_SOFTWARE);
  }

  cache->entries = ALLOCATE(LRUCacheEntry, cap);
  cache->cap = cap;
  cache->len = 0;

  for (size_t i = 0; i < cache->cap; i++) {
    cache->entries[i].key = NULL;
    cache->entries[i].value = NIL_VAL;
  }
}

void free_cache(LRUCache* cache) {
  FREE(LRUCacheEntry, cache->entries);
  cache->entries = NULL;
  cache->cap = 0;
  cache->len = 0;
}

static inline void check_capacity(LRUCache* cache) {
  if (cache->cap > 0) return;

  fprintf(stderr, "attempted to put entry into freed cache");
  exit(EX_SOFTWARE);
}

bool cache_put(LRUCache* cache, ObjString* key, Value val) {
  check_capacity(cache);

  for (size_t i = 0; i < cache->len; i++) {
    LRUCacheEntry* entry = &cache->entries[i];
    if (entry->key == key) return false; // value was already present
  }

  cache->entries[cache->len].key = key;
  cache->entries[cache->len].value = val;
  cache->len++;

  return true;
}

bool cache_get(LRUCache* cache, ObjString* key, Value* out) {
  check_capacity(cache);

  for (size_t i = 0; i < cache->len; i++) {
    LRUCacheEntry* entry = &cache->entries[i];

    if (entry->key == key) { // the value is in the cache, move it up
      *out = entry->value;

      LRUCacheEntry temp = *entry;                    // store the value locally
      memmove(cache->entries + 1, cache->entries, i); // shift all preceding entries back
      *cache->entries = temp;                         // store value at first position

      return true;
    }
  }

  return false;
}
