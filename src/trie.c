#include <stdio.h>
#include <string.h>
#include "trie.h"

#define IS_EMPTY(trie) (trie->data == NULL)
#define CHAR_TO_INDEX(c) ((int) c - (int) 'a')
#define INDEX_TO_CHAR(i) ((char) ((int) 'a' + i))

void init_trie(Trie* trie) {
  trie->data = NULL;
  trie->len = 0;
  trie->cap = 0;
}

void free_trie(Trie* trie) {
  FREE_ARRAY(TrieLeaf*, trie->data, trie->cap);
  init_trie(trie);
}

static void init_leaf(TrieLeaf* leaf) {
  memset(leaf->links, 0, ALPHABET_SIZE * sizeof(TrieLeaf*));
  leaf->terminal = TOKEN__NULL__;
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
  if (IS_EMPTY(trie)) create_leaf(trie);

  TrieLeaf* curr = trie->data;

  for (char c = *element; c != '\0'; c = *++element) {
    curr = get_leaf(trie, curr, c);
  }

  curr->terminal = type;
}

TokenType trie_has(Trie* trie, const char* element, size_t len) {
  if (IS_EMPTY(trie)) return TOKEN__NULL__; // empty tries don't contain anything

  TrieLeaf* curr = trie->data;
  char c;

  for (size_t i = 0; i < len; i++) {
    c = element[i];
    curr = curr->links[CHAR_TO_INDEX(c)];

    if (curr == NULL) return TOKEN__NULL__;
  }

  return curr->terminal;
}

static void dump_leaf(Trie* trie, TrieLeaf* leaf, TrieLeaf* parent, char c) {
#define LEAF_ID(trie, leaf) (((size_t) leaf - (size_t) trie->data) / sizeof(TrieLeaf))

  if (parent != NULL) { // root leaf won't have a parent
    printf("  leaf_%zu [", LEAF_ID(trie, leaf));
    if (leaf->terminal != TOKEN__NULL__) printf("shape=doublecircle, ");
    printf("label=%c]\n", c);
    printf("  leaf_%zu -> leaf_%zu\n", LEAF_ID(trie, parent), LEAF_ID(trie, leaf));
  }

  for (size_t i = 0; i < ALPHABET_SIZE; i++) {
    if (leaf->links[i] == NULL) continue;

    dump_leaf(trie, leaf->links[i], leaf, INDEX_TO_CHAR(i));
  }

#undef LEAF_ID
}

void dump_trie(Trie* trie) {
  if (IS_EMPTY(trie)) {
    printf("graph { empty }");
    return;
  }

  printf("digraph {\n");
  printf("  node [shape=circle]\n");
  printf("  leaf_0 [label=root]\n");
  dump_leaf(trie, trie->data, NULL, '\0');
  printf("}");
}

// ---

#undef IS_EMPTY
#undef CHAR_TO_INDEX
#undef INDEX_TO_CHAR
