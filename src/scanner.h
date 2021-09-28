#ifndef __CLOX_SCANNER_H__
#define __CLOX_SCANNER_H__

#include "common.h"

typedef enum {
  TOKEN__NULL__ = 0, // sentinel value (shouldn't be generated from actual code)

  // single character tokens
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE,
  TOKEN_COMMA, TOKEN_DOT, TOKEN_MINUS, TOKEN_PLUS,
  TOKEN_SEMICOLON, TOKEN_SLASH, TOKEN_STAR,

  // 1-2 character tokens
  TOKEN_BANG, TOKEN_BANG_EQUAL,
  TOKEN_EQUAL, TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL,
  TOKEN_LESS, TOKEN_LESS_EQUAL,

  // literals
  TOKEN_IDENTIFIER, TOKEN_STRING, TOKEN_NUMBER,

  // keywords
  TOKEN_AND, TOKEN_CLASS, TOKEN_ELSE, TOKEN_FALSE,
  TOKEN_FOR, TOKEN_FUN, TOKEN_IF, TOKEN_NIL, TOKEN_OR,
  TOKEN_PRINT, TOKEN_RETURN, TOKEN_SUPER, TOKEN_THIS,
  TOKEN_TRUE, TOKEN_VAR, TOKEN_WHILE,

  // misc.
  TOKEN_ERROR, TOKEN_EOF,
} TokenType;

typedef struct {
  TokenType type;

  const char* start; // pointer to the beginning of the token
  size_t len;        // number of bytes token contains

  size_t line; // line number
} Token;

void init_scanner(const char* source);

Token scan_token();

#endif // __CLOX_SCANNER_H__
