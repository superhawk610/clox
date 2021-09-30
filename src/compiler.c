#include <stdio.h>
#include <stdlib.h>
#include "chunk.h"
#include "compiler.h"
#include "scanner.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

typedef struct {
  Token current;
  Token previous;

  bool had_error; // any error will trip this flag
  bool panicking; // ignore any errors generated while this flag is true
} Parser;

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY,
} Precedence;

typedef void (*ParseFn)();

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

Parser parser;
Chunk* compiling_chunk;

// down the road, chunks may live elsewhere
static Chunk* current_chunk() { return compiling_chunk; }

static void init_parser() {
  parser.had_error = false;
  parser.panicking = false;
}

static void error_at(Token* token, const char* message) {
  if (parser.panicking) return; // ignore errors if we're already panicking

  parser.panicking = true;
  parser.had_error = true;

  fprintf(stderr, "[line %zu] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // do nothing
  } else {
    fprintf(stderr, " at '%.*s'", (int) token->len, token->start);
  }

  fprintf(stderr, ": %s\n", message);
}

// an error was encountered at the token we just consumed
static void error(const char* message) {
  error_at(&parser.previous, message);
}

// an error was encountered at the token we're about to consume
static void error_at_current(const char* message) {
  error_at(&parser.current, message);
}

static void advance() {
  parser.previous = parser.current;

  for (;;) {
    // scan tokens, stopping after a valid token is generated
    // (errors will be marked and consumed until a valid token is encountered)
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR) break;

    error_at_current(parser.current.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  error_at_current(message);
}

static void emit_byte(uint8_t byte) {
  write_chunk(current_chunk(), byte, parser.previous.line);
}

static inline void emit_bytes(uint8_t a, uint8_t b) {
  emit_byte(a);
  emit_byte(b);
}

static void emit_constant(Value val) {
  uint16_t constant = add_constant(current_chunk(), val);

  if (constant > UINT8_MAX) {
    emit_byte(OP_CONST);
    emit_bytes(/* hi */ constant >> 8, /* lo */ constant);
  } else {
    emit_bytes(OP_CONST, constant);
  }
}

static void emit_return() {
  emit_byte(OP_RETURN);
}

static void end_compiler() {
  emit_return();

#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    disasm_chunk(current_chunk(), "code [debug]");
  }
#endif
}

static void parse_precedence(Precedence prec);
static ParseRule* get_rule(TokenType type);
static void expression();

static void grouping() {
  // the opening TOKEN_LEFT_PAREN has just been consumed
  expression(); // compile expression body
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void number() {
  double val = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(val));
}

static void literal() {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL:   emit_byte(OP_NIL); break;
    case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
    default: return; // unreachable
  }
}

static void unary() {
  TokenType op_type = parser.previous.type;

  parse_precedence(PREC_UNARY); // compile the operand (use PREC_UNARY to allow
                                // for double-unary ops in sequence, e.g. !!)

  switch (op_type) {
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return; // unreachable
  }
}

static void binary() {
  TokenType op_type = parser.previous.type;
  ParseRule* rule = get_rule(op_type);

  // parse the right operand with 1 _higher_ precedence so that
  // binary operations are left-associtive; in other words, we want
  //
  //     1 + 2 + 3 + 4
  //
  // to be effectively parsed as
  //
  //     (((1 + 2) + 3) + 4)
  //
  parse_precedence((Precedence) (rule->precedence + 1));

  switch (op_type) {
    case TOKEN_PLUS:  emit_byte(OP_ADD); break;
    case TOKEN_MINUS: emit_byte(OP_SUBTRACT); break;
    case TOKEN_STAR:  emit_byte(OP_MULTIPLY); break;
    case TOKEN_SLASH: emit_byte(OP_DIVIDE); break;
    default: return; // unreachable
  }
}

static void parse_precedence(Precedence prec) {
  advance();

  ParseFn prefix_rule = get_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expected expression.");
    return;
  }

  prefix_rule(); // start by following a prefix rule...

  // ...then follow infix rules, as long as precedence allows
  // (keep going so long as the ops have the same/higher precedence)
  while (prec <= get_rule(parser.current.type)->precedence) {
    advance();
    get_rule(parser.previous.type)->infix();
  }
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT); // parse with the lowest precedence, so
                                     // all higher-precedence expressions are
                                     // parsed as well
}

/*
 * These rules define a [Pratt parser][1]. Pratt parsers run
 * left-to-right and parse tokens as they're encountered. It
 * supports prefix, infix, and mixfix (both prefix and infix)
 * operators. The parser begins by parsing a token as a prefix
 * operator (all expresssions must start with a token that
 * has an associated prefix operator), then attempting to
 * parse zero or more infix operators at the same or higher
 * precedence). For example, the expression
 *
 *     1 + 2 * 3
 *
 *     1. parse `1` as prefix with `number()`
 *
 *     2. parse `+` as infix with `binary()`, using `1`
 *        as the left operand
 *
 *     3. parse the right operand of `+`, starting with...
 *
 *     4. parse `2` as prefix with `number()`
 *
 *     5. parse `*` as infix with `binary()`, using the
 *        entire expression thus far as the left operand
 *
 *     6. parse the right operand of `*`, starting with...
 *
 *     7. parse `3` as prefix with `number()`
 *
 * The resulting bytecode for this will be
 *
 *     OP_CONST 1
 *     OP_CONST 2
 *     OP_CONST 3
 *     OP_MULTIPLY
 *     OP_ADD
 *
 * Note that operators don't output their corresponding op
 * to bytecode until they've completed parsing their operands
 * (which are output to bytecode when parsed). This means that
 * ops are rearranged in bytecode and resemble RPN (reverse
 * Polish notation).
 *
 *     1 + 2
 *
 * becomes
 *
 *     1 2 +
 *
 *     OP_CONST 1
 *     OP_CONST 2
 *     OP_ADD
 *
 * NOTE: All prefix operators in Lox have the same precedence,
 * so this table tracks the precedence of the _infix_ operators
 * specifically.
 *
 * [1]: http://craftinginterpreters.com/compiling-expressions.html#a-pratt-parser
 */
ParseRule rules[] = {
/*+---------------------+-----------+--------+-------------+
  | token               | prefix    | infix  | precedence  |
  +---------------------+-----------+--------+-------------+*/
  [TOKEN_LEFT_PAREN]    = {grouping,  NULL,    PREC_NONE},
  [TOKEN_RIGHT_PAREN]   = {NULL,      NULL,    PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {NULL,      NULL,    PREC_NONE},
  [TOKEN_RIGHT_BRACE]   = {NULL,      NULL,    PREC_NONE},
  [TOKEN_COMMA]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_DOT]           = {NULL,      NULL,    PREC_NONE},
  [TOKEN_MINUS]         = {unary,     binary,  PREC_TERM},
  [TOKEN_PLUS]          = {NULL,      binary,  PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,      NULL,    PREC_NONE},
  [TOKEN_SLASH]         = {NULL,      binary,  PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,      binary,  PREC_FACTOR},
  [TOKEN_BANG]          = {NULL,      NULL,    PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,      NULL,    PREC_NONE},
  [TOKEN_EQUAL]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,      NULL,    PREC_NONE},
  [TOKEN_GREATER]       = {NULL,      NULL,    PREC_NONE},
  [TOKEN_GREATER_EQUAL] = {NULL,      NULL,    PREC_NONE},
  [TOKEN_LESS]          = {NULL,      NULL,    PREC_NONE},
  [TOKEN_LESS_EQUAL]    = {NULL,      NULL,    PREC_NONE},
  [TOKEN_IDENTIFIER]    = {NULL,      NULL,    PREC_NONE},
  [TOKEN_STRING]        = {NULL,      NULL,    PREC_NONE},
  [TOKEN_NUMBER]        = {number,    NULL,    PREC_NONE},
  [TOKEN_AND]           = {NULL,      NULL,    PREC_NONE},
  [TOKEN_CLASS]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_ELSE]          = {NULL,      NULL,    PREC_NONE},
  [TOKEN_FALSE]         = {literal,   NULL,    PREC_NONE},
  [TOKEN_FOR]           = {NULL,      NULL,    PREC_NONE},
  [TOKEN_FUN]           = {NULL,      NULL,    PREC_NONE},
  [TOKEN_IF]            = {NULL,      NULL,    PREC_NONE},
  [TOKEN_NIL]           = {literal,   NULL,    PREC_NONE},
  [TOKEN_OR]            = {NULL,      NULL,    PREC_NONE},
  [TOKEN_PRINT]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_RETURN]        = {NULL,      NULL,    PREC_NONE},
  [TOKEN_SUPER]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_THIS]          = {NULL,      NULL,    PREC_NONE},
  [TOKEN_TRUE]          = {literal,   NULL,    PREC_NONE},
  [TOKEN_VAR]           = {NULL,      NULL,    PREC_NONE},
  [TOKEN_WHILE]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_ERROR]         = {NULL,      NULL,    PREC_NONE},
  [TOKEN_EOF]           = {NULL,      NULL,    PREC_NONE},
};

static ParseRule* get_rule(TokenType type) { return &rules[type]; }

bool compile(const char* source, Chunk* chunk) {
  compiling_chunk = chunk;

  init_scanner(source);
  init_parser();

  advance();
  expression();
  consume(TOKEN_EOF, "Expected end of expression.");
  end_compiler();

  return !parser.had_error;
}
