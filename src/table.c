#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "table.h"

// load factor
#define TABLE_LOAD_MAX 0.75

#define TOMBSTONE() (BOOL_VAL(true))
#define TABLE_IS_EMPTY(tab) (tab->len == 0)

void init_table(Table* tab) {
  tab->entries = NULL;
  tab->len = 0;
  tab->cap = 0;
}

void free_table(Table* tab) {
  FREE_ARRAY(Entry, tab->entries, tab->cap);
  init_table(tab);
}

// Walk the array of entries starting from the key's hash, noting the
// first tombstone encountered. If an entry with a matching key
// is found, return that. If not, continue traversing until an
// empty bucket is encountered. If any tombstones were encountered,
// return the first. If not, return the empty bucket.
//
// For example, when finding entry `x` in the following
//
//      0  1  2  3
//     [x][ ][T][ ]  #=> 0 (found matching entry)
//
//     [ ][ ][T][ ]  #=> 0 (first empty bucket)
//
//     [-][ ][T][ ]  #=> 1 (first empty bucket)
//
//     [-][-][T][ ]  #=> 2 (first tombstone)
//
static Entry* find_entry(Entry* entries, size_t cap, ObjString* key) {
  Entry* tombstone = NULL;
  uint32_t bucket_idx = key->hash % cap;

  for (;;) {
    Entry* entry = &entries[bucket_idx];

    if (entry->key == NULL) {
      if (IS_NIL(entry->value)) {    // empty entry
        return tombstone != NULL ? tombstone : entry;
      } else {                       // tombstone
        if (tombstone == NULL) tombstone = entry;
      }
    } else if (entry->key == key) {
      return entry;                  // found it!
    }

    bucket_idx = (bucket_idx + 1) % cap; // wrap around if we hit
                                         // the last bucket
  }
}

static void adjust_capacity(Table* tab, size_t cap) {
  Entry* entries = ALLOCATE(Entry, cap);

  // initialize new bucket storage
  for (size_t i = 0; i < cap; i++) {
    entries[i].key = NULL;
    entries[i].value = NIL_VAL;
  }

  // copy over existing bucket entries
  tab->len = 0; // reset to zero...
  for (size_t i = 0; i < tab->cap; i++) {
    Entry* entry = &tab->entries[i];
    if (entry->key == NULL) continue; // skip empty buckets

    Entry* dest = find_entry(entries, cap, entry->key);
    dest->key = entry->key;
    dest->value = entry->value;
    tab->len++; // ...then increment for non-tombstone entries
  }

  // free previous storage
  FREE_ARRAY(Entry, tab->entries, tab->cap);

  // update table to point at new storage
  tab->entries = entries;
  tab->cap = cap;
}

bool table_set(Table* tab, ObjString* key, Value val) {
  if (tab->len + 1 > tab->cap * TABLE_LOAD_MAX) {
    size_t cap = GROW_CAPACITY(tab->cap);
    adjust_capacity(tab, cap);
  }

  Entry* entry = find_entry(tab->entries, tab->cap, key);
  bool is_new_key = entry->key == NULL;

  // only increment length if we _aren't_ replacing a tombstone
  if (is_new_key && /* isn't tombstone */ IS_NIL(entry->value)) {
    tab->len++;
  }

  entry->key = key;
  entry->value = val;
  return is_new_key;
}

bool table_get(Table* tab, ObjString* key, Value* out) {
  if (TABLE_IS_EMPTY(tab)) return false; // empty tables don't contain anything

  Entry* entry = find_entry(tab->entries, tab->cap, key);
  if (entry->key == NULL) return false; // key isn't contained in table

  *out = entry->value; // found key, write value to out pointer
  return true;
}

bool table_delete(Table* tab, ObjString* key) {
  if (TABLE_IS_EMPTY(tab)) return false;

  Entry* entry = find_entry(tab->entries, tab->cap, key);
  if (entry->key == NULL) return false;

  entry->key = NULL;
  entry->value = TOMBSTONE();
  return true;
}

// similar to table_find_key, but searches with a raw C string
ObjString* table_find_string(Table* tab, const char* chars, size_t len, uint32_t hash) {
  if (TABLE_IS_EMPTY(tab)) return NULL;

  uint32_t bucket_idx = hash % tab->cap;
  for (;;) {
    Entry* entry = &tab->entries[bucket_idx];

    if (entry->key == NULL) {
      // empty (non-tombstone) bucket, string must not be here
      if (IS_NIL(entry->value)) return NULL;
    } else if (entry->key->len == len &&
               memcmp(entry->key->chars, chars, len) == 0) {
      // key with same length and chars must be a match
      return entry->key;
    }

    bucket_idx = (bucket_idx + 1) % tab->cap;
  }
}

void table_merge(Table* src, Table* dest) {
  for (size_t i = 0; i < src->cap; i++) {
    Entry* entry = &src->entries[i];
    if (entry->key != NULL) {
      table_set(dest, entry->key, entry->value);
    }
  }
}

static void dump_bucket(Entry* bucket, size_t cap, size_t bucket_idx) {
  if (bucket_idx > 0) printf(" | ");
  if (bucket->key != NULL) {
    printf("<bucket_%zu> \\\"%s\\\"\\nhash: %zu", bucket_idx,
                                                  bucket->key->chars,
                                                  bucket->key->hash % cap);
  } else if (IS_NIL(bucket->value)) {
    printf("<bucket_%zu>", bucket_idx); // empty bucket
  } else {
    printf("<bucket_%zu> [T]", bucket_idx);
  }
}

static void dump_bucket_value(Entry* bucket, size_t bucket_idx) {
  if (bucket->key == NULL) return; // skip empty buckets and tombstones

  printf("  value_%zu [label=\"", bucket_idx);
  print_value(bucket->value);
  printf("\" shape=circle]\n");
  printf("  table:bucket_%zu -- value_%zu\n", bucket_idx, bucket_idx);
}

/*
 * Generate a Graphviz graph that looks something like
 *
 *     graph {
 *       node [shape=record]
 *       table [label="<bucket_0> key | <bucket_1> | <bucket_2> [T]"]
 *       value_0 [label="value" shape=circle]
 *       table:bucket_0 -- value_0
 *     }
 *
 */
void dump_table(Table* tab) {
  if (TABLE_IS_EMPTY(tab)) {
    printf("graph { empty [shape=box] }");
    return;
  }

  printf("graph {\n");
  printf("  node [shape=record]\n");
  printf("  table [label=\"");
  for (size_t i = 0; i < tab->cap; i++) {
    dump_bucket(&tab->entries[i], tab->cap, i); // heh, don't spill it
  }
  printf("\"]\n");
  for (size_t i = 0; i < tab->cap; i++) {
    dump_bucket_value(&tab->entries[i], i);
  }
  printf("}");
}

// ---

#undef TOMBSTONE
#undef TABLE_IS_EMPTY
