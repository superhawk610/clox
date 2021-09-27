#include <stdio.h>
#include <string.h>
#include "common.h"
#include "scanner.h"

/**
 * Scanner that tracks two indices, the start and current pointers
 * into the scanning window for a single token.
 *
 *     [p] [r] [i] [n] [t] [ ] [b] [a] [c] [o] [n] [;] [0]
 *                              ^           ^
 *                            start       current
 */
typedef struct {
  const char* start;
  const char* current;

  size_t line;
} Scanner;

Scanner scanner;

void init_scanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

static bool is_at_end() {
  return *scanner.current == '\0'; // source is null-terminated
}

static Token make_token(TokenType type) {
  Token tok;

  tok.type = type;
  tok.start = scanner.start;
  tok.len = (size_t)(scanner.current - scanner.start);
  tok.line = scanner.line;

  return tok;
}

static Token error_token(const char* message) {
  Token tok;

  tok.type = TOKEN_ERROR;
  tok.start = message;
  tok.len = (size_t) strlen(message);
  tok.line = scanner.line;

  return tok;
}

static char advance() {
  scanner.current++;
  return scanner.current[-1];
}

Token scan_token() {
  scanner.start = scanner.current;

  if (is_at_end()) return make_token(TOKEN_EOF);

  switch (advance()) {
    case '(': return make_token(TOKEN_LEFT_PAREN);
    case ')': return make_token(TOKEN_RIGHT_PAREN);
    case '{': return make_token(TOKEN_LEFT_BRACE);
    case '}': return make_token(TOKEN_RIGHT_BRACE);
    case ';': return make_token(TOKEN_SEMICOLON);
    case ',': return make_token(TOKEN_COMMA);
    case '.': return make_token(TOKEN_DOT);
    case '-': return make_token(TOKEN_MINUS);
    case '+': return make_token(TOKEN_PLUS);
    case '/': return make_token(TOKEN_SLASH);
    case '*': return make_token(TOKEN_STAR);
  }

  return error_token("Unexpected character.");
}
