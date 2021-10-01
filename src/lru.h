#ifndef __CLOX_LRU_H__
#define __CLOX_LRU_H__

#include "common.h"
#include "object.h"
#include "value.h"

typedef struct {
  ObjString* key;
  Value value;
} LRUCacheEntry;

/**
 * Least-recently used (LRU) cache.
 *
 * Retrievals walk the inner array linearly, writing the
 * value to the out pointer if encountered. When retrieving
 * a value, it's corresponding entry is moved to the front
 * of the array, so subsequent retrievals will be faster.
 *
 * As values are moved to the front, they push all subsequent
 * values back by one, dropping any values beyond the cache's
 * capacity.
 *
 * This cache performs best with lookups grouped by common keys
 * (which is hopefully a good fit for global variable storage).
 */
typedef struct {
  LRUCacheEntry* entries;

  size_t len; // number of entries in the cache
  size_t cap; // available entry slots
} LRUCache;

void init_cache(LRUCache* cache, size_t cap);

void free_cache(LRUCache* cache);

/** @return true if the value was added to the cache (false if it was already present) */
bool cache_put(LRUCache* cache, ObjString* key, Value val);

/** @return true if the value was present in the cache */
bool cache_get(LRUCache* cache, ObjString* key, Value* out);

#endif // __CLOX_LRU_H__
