#ifndef __CLOX_TABLE_H__
#define __CLOX_TABLE_H__

#include "common.h"
#include "value.h"

// TODO: support numeric keys
typedef struct {
  ObjString* key;
  Value value;
} Entry;

/**
 * Basic implementation of a hash table (hashmap) using
 * open addressing. So-called "buckets" are allocated
 * and begin empty, then are filled with a single key-value
 * pair when storing a new value. A hash function is
 * used to determine the bucket offset to store a value
 * in, and if that bucket is already filled (if there is
 * a hash collision), the subsequent buckets will be
 * traversed until an empty one is found, looping back around
 * to the start of the table if no empty buckets are found.
 *
 * Key deletion is done using tombstones, marking previously-
 * filled buckets so they don't offset the open addressing
 * scheme. Tombstones will remain until they're overwritten
 * by a new value. They may also be removed by copying the
 * buckets over to a new table, since they'll be excluded
 * and won't be present in the new table.
 *
 *     [ ][a][ ][T][ ][b][c][ ]  8 buckets
 *         |     |     |  |
 *         |     |     |  |- "baz"
 *         |     |     |
 *         |     |     |- "bar"
 *         |     |
 *         |     |- tombstone
 *         |
 *         |- "foo"
 *
 *     a: "foo", hash = 1
 *     b: "bar", hash = 13 % 8 = 5
 *     c: "baz", hash = 5 (shifted over to next empty bucket)
 *
 */
typedef struct {
  // dynamic array of hash buckets
  Entry* entries;

  size_t len; // number of filled buckets + tombstones
  size_t cap; // available slots (including empty buckets)
} Table;

void init_table(Table* tab);

void free_table(Table* tab);

/** @return true if a new key was added, false if an existing entry was overwritten */
bool table_set(Table* tab, ObjString* key, Value val);

/** @return true if the key was contained in the table */
bool table_get(Table* tab, ObjString* key, Value* out);

/** @return true if the key was contained in the table */
bool table_delete(Table* tab, ObjString* key);

ObjString* table_find_string(Table* tab, const char* chars, size_t len, uint32_t hash);

void table_merge(Table* src, Table* dest);

void dump_table(Table* tab);

#endif // __CLOX_TABLE_H__
