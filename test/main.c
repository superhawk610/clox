#include "common.h"
#include "trie.h"

int main() {
  test_trie();

  fprintf(stderr, "\n" ANSI_BGGreen ANSI_Black "  \u2713  " ANSI_Reset " " \
                  ANSI_Green "All tests passed!" ANSI_Reset "\n");

  return 0;
}
