#include "../src/trie.h"
#include "common.h"
#include "trie.h"

#define FOO TOKEN_IF
#define BAR TOKEN_AND
#define BAZ TOKEN_OR

void test_trie() {
  Trie trie;

  init_trie(&trie);

  trie_push(&trie, "foo", FOO);
  trie_push(&trie, "bar", BAR);
  trie_push(&trie, "baz", BAZ);

  refute(trie_has(&trie, "f", 1));
  refute(trie_has(&trie, "fo", 2));
  assert(trie_has(&trie, "foo", 3) == FOO);
  assert(trie_has(&trie, "bar", 3) == BAR);
  assert(trie_has(&trie, "baz", 3) == BAZ);
  refute(trie_has(&trie, "bazz", 4));
  refute(trie_has(&trie, "qux", 3));

  free_trie(&trie);
}

// ---

#undef FOO
#undef BAR
#undef BAZ
