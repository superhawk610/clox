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
Trie keywords;

#define IS_AT_END() (*scanner.current == '\0')
#define ADVANCE() (*scanner.current++)
#define PEEK() (*scanner.current)
#define PEEK_NEXT() (IS_AT_END() ? '\0' : scanner.current[1])

#define IS_DIGIT(c) (c >= '0' && c <= '9')
#define IS_ALPHA(c) \
  ((c >= 'a' && c <= 'z') || \
   (c >= 'A' && c <= 'Z') || \
   c == '_')

static void init_keywords() {
  static bool once;
  if (once) return;
  once = true;

  init_trie(&keywords);
  trie_push(&keywords, "and", TOKEN_AND);
  trie_push(&keywords, "class", TOKEN_CLASS);
  trie_push(&keywords, "else", TOKEN_ELSE);
  trie_push(&keywords, "false", TOKEN_FALSE);
  trie_push(&keywords, "for", TOKEN_FOR);
  trie_push(&keywords, "fun", TOKEN_FUN);
  trie_push(&keywords, "if", TOKEN_IF);
  trie_push(&keywords, "nil", TOKEN_NIL);
  trie_push(&keywords, "or", TOKEN_OR);
  trie_push(&keywords, "print", TOKEN_PRINT);
  trie_push(&keywords, "return", TOKEN_RETURN);
  trie_push(&keywords, "super", TOKEN_SUPER);
  trie_push(&keywords, "this", TOKEN_THIS);
  trie_push(&keywords, "true", TOKEN_TRUE);
  trie_push(&keywords, "var", TOKEN_VAR);
  trie_push(&keywords, "while", TOKEN_WHILE);
}

void init_scanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;

  init_keywords(); // memoized to only execute once
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

static char match(char expected) {
  if (IS_AT_END()) return false;
  if (PEEK() != expected) return false;

  // the expected character was next in the input stream,
  // consume it and continue parsing
  ADVANCE();
  return true;
}

static void skip_whitespace() {
  for (;;) {
    switch (PEEK()) {
      case ' ':
      case '\r':
      case '\t':
        ADVANCE();
        break;
      case '\n':
        scanner.line++;
        ADVANCE();
        break;
      case '/': // comments are whitespace, right? ;)
        if (PEEK_NEXT() != '/') return; // not a comment

        // consume and discard everything through the end of the line
        while (PEEK() != '\n' && !IS_AT_END()) ADVANCE();
        break;
      default:
        return;
    }
  }
}

static Token string() {
  while (PEEK() != '"' && !IS_AT_END()) {
    if (PEEK() == '\n') scanner.line++;
    ADVANCE();
  }

  if (IS_AT_END()) return error_token("Unterminated string literal.");

  ADVANCE(); // skip the closing quotation mark
  return make_token(TOKEN_STRING);
}

static Token number() {
  while (IS_DIGIT(PEEK())) ADVANCE();

  // look for a fractional part
  if (PEEK() == '.' && IS_DIGIT(PEEK_NEXT())) {
    ADVANCE(); // skip the decimal point
    while (IS_DIGIT(PEEK())) ADVANCE();
  }

  return make_token(TOKEN_NUMBER);
}

static TokenType identifier_type() {
  TokenType type = trie_has(&keywords, scanner.start, scanner.current - scanner.start);
  return type == TOKEN__NULL__ ? TOKEN_IDENTIFIER : type;
}

static Token identifier() {
  while (IS_ALPHA(PEEK()) || IS_DIGIT(PEEK())) ADVANCE();
  return make_token(identifier_type());
}

Token scan_token() {
  skip_whitespace();

  scanner.start = scanner.current;

  if (IS_AT_END()) return make_token(TOKEN_EOF);

  char c = ADVANCE();

  if (IS_ALPHA(c)) return identifier();
  if (IS_DIGIT(c)) return number();

  switch (c) {
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

    case '!': return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=': return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<': return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
    case '>': return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);

    case '"': return string();
  }

  return error_token("Unexpected character.");
}

// ---

#undef IS_AT_END
#undef ADVANCE
#undef PEEK
#undef PEEK_NEXT
#undef IS_DIGIT
#undef IS_ALPHA
