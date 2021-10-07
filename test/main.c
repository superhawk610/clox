#include "common.h"
#include "fixtures.h"
#include "../src/logger.h"
#include "trie.h"

static void __test_success(const char* message) {
  fprintf(stderr, ANSI_BGGreen ANSI_Black "  \u2713  " ANSI_Reset " " \
                  ANSI_Green "%s" ANSI_Reset "\n", message);
}

int main() {
  init_logger();

  test_fixtures();
  __test_success("test/fixtures");

  test_trie();
  __test_success("test/trie");

  __test_success("All tests passed!");
  return 0;
}
