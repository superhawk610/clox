#include "../src/trie.h"
#include "common.h"
#include "trie.h"

#define FOO TOKEN_IF
#define BAR TOKEN_AND
#define BAZ TOKEN_OR
#define NOT_FOUND TOKEN__NULL__

void test_trie() {
  Trie trie;

  init_trie(&trie);

  trie_push(&trie, "foo", FOO);
  trie_push(&trie, "bar", BAR);
  trie_push(&trie, "baz", BAZ);

  assert(trie_has(&trie, "f", 1) == NOT_FOUND);
  assert(trie_has(&trie, "fo", 2) == NOT_FOUND);
  assert(trie_has(&trie, "foo", 3) == FOO);
  assert(trie_has(&trie, "bar", 3) == BAR);
  assert(trie_has(&trie, "baz", 3) == BAZ);
  assert(trie_has(&trie, "bazz", 4) == NOT_FOUND);
  assert(trie_has(&trie, "qux", 3) == NOT_FOUND);

  free_trie(&trie);
}

// ---

#undef FOO
#undef BAR
#undef BAZ
#undef NOT_FOUND
