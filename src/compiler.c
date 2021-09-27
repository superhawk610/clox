#include <stdio.h>
#include "common.h"
#include "compiler.h"
#include "scanner.h"

void compile(const char* source) {
  init_scanner(source);

  size_t line = 0;
  for (;;) {
    Token tok = scan_token();

    if (tok.line != line) {
      printf("%4zu ", tok.line);
      line = tok.line;
    } else {
      printf("   | ");
    }
    printf("%2d '%.*s'\n", tok.type, tok.len, tok.start);

    if (tok.type == TOKEN_EOF) break;
  }
}
