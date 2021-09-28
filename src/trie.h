#ifndef __CLOX_TRIE_H__
#define __CLOX_TRIE_H__

#include "common.h"
#include "memory.h"
#include "scanner.h"

// we only care about lowercase ASCII characters
#define ALPHABET_SIZE 26

typedef struct TrieLeaf {
  struct TrieLeaf* links[ALPHABET_SIZE]; // fixed-size array of links to other leaf nodes;
                                         // if a character isn't valid to follow, its link
                                         // will be NULL

  TokenType terminal; // the contained value, if the leaf terminates a contained value
} TrieLeaf;

/**
 * Prefix reTRIEval data structure.
 *
 * Given a set of zero or more string inputs that may include overlapping
 * character prefixes, constructs a data structure suitable for fast membership
 * lookups. Comparisons will be done character-by-character, short-circuiting
 * if no possible matches remain.
 *
 *        b     f
 *        |     |
 *        a     o
 *       / \    |
 *     [r] [z] [o]
 */
typedef struct {
  // dynamic array containing leaf nodes
  TrieLeaf* data;

  size_t len; // the number of leaves in the trie
  size_t cap; // the available spots for leaves
} Trie;

void init_trie(Trie* trie);

void trie_push(Trie* trie, const char* element, TokenType type);

/** Returns TOKEN__NULL__ if the element wasn't contained in the trie. */
TokenType trie_has(Trie* trie, const char* element, size_t len);

void free_trie(Trie* trie);

#endif // __CLOX_TRIE_H__
