#include <string.h>
#include "trie.h"

#define CHAR_TO_INDEX(c) ((size_t) c - (size_t) 'a')

void init_trie(Trie* trie) {
  trie->data = NULL;
  trie->len = 0;
  trie->cap = 0;
}

static void init_leaf(TrieLeaf* leaf) {
  memset(leaf, 0, sizeof(TrieLeaf));
}

static TrieLeaf* create_leaf(Trie* trie) {
  if (trie->len == trie->cap) {
    size_t old_cap = trie->cap;
    trie->cap = GROW_CAPACITY(old_cap);
    trie->data = GROW_ARRAY(TrieLeaf, trie->data, old_cap, trie->cap);
  }

  TrieLeaf* leaf = &trie->data[trie->len++];
  init_leaf(leaf);
  return leaf;
}

static TrieLeaf* get_leaf(Trie* trie, TrieLeaf* curr, char c) {
  TrieLeaf* link = curr->links[CHAR_TO_INDEX(c)];
  if (link) return link;

  // if the leaf doesn't already exist, create it
  link = create_leaf(trie);
  curr->links[CHAR_TO_INDEX(c)] = link;
  return link;
}

void trie_push(Trie* trie, const char* element, TokenType type) {
  // if this is the first time we've pushed, initialize the root leaf
  if (trie->data == NULL) create_leaf(trie);

  TrieLeaf* curr = trie->data;

  for (char c = *element; c != '\0'; c = *++element) {
    curr = get_leaf(trie, curr, c);
  }

  curr->terminal = type;
}

TokenType trie_has(Trie* trie, const char* element, size_t len) {
  if (trie->data == NULL) return TOKEN__NULL__; // empty tries don't contain anything

  TrieLeaf* curr = trie->data;
  char c;

  for (size_t i = 0; i < len; i++) {
    c = element[i];
    curr = curr->links[CHAR_TO_INDEX(c)];

    if (curr == NULL) return TOKEN__NULL__;
  }

  return curr->terminal;
}

void free_trie(Trie* trie) {
  FREE_ARRAY(TrieLeaf*, trie->data, trie->cap);
  init_trie(trie);
}

#undef CHAR_TO_INDEX
