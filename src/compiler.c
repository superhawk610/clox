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

typedef void (*ParseFn)(bool can_assign);

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

static bool check(TokenType type) {
  return parser.current.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;

  advance();
  return true;
}

static void emit_byte(uint8_t byte) {
  write_chunk(current_chunk(), byte, parser.previous.line);
}

static inline void emit_bytes(uint8_t a, uint8_t b) {
  emit_byte(a);
  emit_byte(b);
}

static void emit_constant_op(uint16_t constant, OpCode short_op, OpCode long_op) {
  if (constant > UINT8_MAX) {
    emit_byte(long_op);
    emit_bytes(/* hi */ constant >> 8, /* lo */ constant);
  } else {
    emit_bytes(short_op, constant);
  }
}

static void emit_constant(Value val) {
  uint16_t constant = add_constant(current_chunk(), val);
  emit_constant_op(constant, OP_CONST, OP_CONST_LONG);
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
static void statement();
static void declaration();

static void grouping(bool can_assign) {
  // the opening TOKEN_LEFT_PAREN has just been consumed
  expression(); // compile expression body
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void number(bool can_assign) {
  double val = strtod(parser.previous.start, NULL);
  emit_constant(NUMBER_VAL(val));
}

static void string(bool can_assign) {
  // trim the leading and trailing quotation marks
  //
  //     "hello, world!"
  //      ^           ^
  //      1        (len-2)
  //
  emit_constant(OBJ_VAL((Obj*) copy_string(parser.previous.start + 1,
                                           parser.previous.len - 2)));
}

static uint16_t identifier_constant(Token* name) {
  Value str = OBJ_VAL((Obj*) copy_string(name->start, name->len));
  return add_constant(current_chunk(), str);
}

static void named_variable(Token name, bool can_assign) {
  uint16_t arg = identifier_constant(&name);

  if (can_assign && match(TOKEN_EQUAL)) { // if followed by `=` (i.e. a = 1),
    expression();                         // it's variable assignment...
    emit_constant_op(arg, OP_SET_GLOBAL, OP_SET_GLOBAL_LONG);
  } else {                                // ...otherwise, it's just variable access
    emit_constant_op(arg, OP_GET_GLOBAL, OP_GET_GLOBAL_LONG);
  }
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void literal(bool can_assign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emit_byte(OP_FALSE); break;
    case TOKEN_NIL:   emit_byte(OP_NIL); break;
    case TOKEN_TRUE:  emit_byte(OP_TRUE); break;
    default: return; // unreachable
  }
}

static void unary(bool can_assign) {
  TokenType op_type = parser.previous.type;

  parse_precedence(PREC_UNARY); // compile the operand (use PREC_UNARY to allow
                                // for double-unary ops in sequence, e.g. !!)

  switch (op_type) {
    case TOKEN_BANG:  emit_byte(OP_NOT); break;
    case TOKEN_MINUS: emit_byte(OP_NEGATE); break;
    default: return; // unreachable
  }
}

static void binary(bool can_assign) {
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

    case TOKEN_BANG_EQUAL:    emit_bytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emit_byte(OP_EQUAL); break;
    case TOKEN_GREATER:       emit_byte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emit_bytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emit_byte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emit_bytes(OP_GREATER, OP_NOT); break;

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

  bool can_assign = prec <= PREC_ASSIGNMENT; // only allow assignment when parsing an
                                             // initializer or top-level expression

  prefix_rule(can_assign); // start by following a prefix rule...

  // ...then follow infix rules, as long as precedence allows
  // (keep going so long as the ops have the same/higher precedence)
  while (prec <= get_rule(parser.current.type)->precedence) {
    advance();
    get_rule(parser.previous.type)->infix(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static uint16_t parse_variable(const char* err_message) {
  consume(TOKEN_IDENTIFIER, err_message);
  return identifier_constant(&parser.previous);
}

static void define_variable(uint8_t global) {
  emit_bytes(OP_DEF_GLOBAL, global);
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT); // parse with the lowest precedence, so
                                     // all higher-precedence expressions are
                                     // parsed as well
}

static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(OP_POP); // evaluate the expression, then discard the result
}

static void print_statement() {
  expression(); // evaluate the `print` statement's operand, placing it on the stack
  consume(TOKEN_SEMICOLON, "Expected ';' after value.");
  emit_byte(OP_PRINT);
}

static void statement() {
  if (match(TOKEN_PRINT)) print_statement();
  else                    expression_statement();
}

static void synchronize() {
  parser.panicking = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return; // we've passed a statement boundary,
                                                         // let's try resuming compilation, or...
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_FUN:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_PRINT:
      case TOKEN_RETURN:
        return;       // ...we're about to begin a new statement,
                      // let's try there

      default: break; // keep looking
    }

    advance();
  }
}

static void var_declaration() {
  uint8_t global = parse_variable("Expected variable name.");

  if (match(TOKEN_EQUAL)) expression();      // read initializer expression, or
  else                    emit_byte(OP_NIL); // default to nil, if none is provided

  consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  define_variable(global);
}

static void declaration() {
  if (match(TOKEN_VAR)) var_declaration();
  else                  statement();

  // if things went awry, try to find a synchronization point
  // and re-enable normal compilation (on a statement boundary)
  if (parser.panicking) synchronize();
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
/*+---------------------+-----------+--------+-----------------+
  | token               | prefix    | infix  | precedence      |
  +---------------------+-----------+--------+-----------------+*/
  [TOKEN_LEFT_PAREN]    = {grouping,  NULL,    PREC_NONE       },
  [TOKEN_RIGHT_PAREN]   = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_LEFT_BRACE]    = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_RIGHT_BRACE]   = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_COMMA]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_DOT]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_MINUS]         = {unary,     binary,  PREC_TERM       },
  [TOKEN_PLUS]          = {NULL,      binary,  PREC_TERM       },
  [TOKEN_SEMICOLON]     = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_SLASH]         = {NULL,      binary,  PREC_FACTOR     },
  [TOKEN_STAR]          = {NULL,      binary,  PREC_FACTOR     },
  [TOKEN_BANG]          = {unary,     NULL,    PREC_NONE       },
  [TOKEN_BANG_EQUAL]    = {NULL,      binary,  PREC_EQUALITY   },
  [TOKEN_EQUAL]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_EQUAL_EQUAL]   = {NULL,      binary,  PREC_EQUALITY   },
  [TOKEN_GREATER]       = {NULL,      binary,  PREC_COMPARISON },
  [TOKEN_GREATER_EQUAL] = {NULL,      binary,  PREC_COMPARISON },
  [TOKEN_LESS]          = {NULL,      binary,  PREC_COMPARISON },
  [TOKEN_LESS_EQUAL]    = {NULL,      binary,  PREC_COMPARISON },
  [TOKEN_IDENTIFIER]    = {variable,  NULL,    PREC_NONE       },
  [TOKEN_STRING]        = {string,    NULL,    PREC_NONE       },
  [TOKEN_NUMBER]        = {number,    NULL,    PREC_NONE       },
  [TOKEN_AND]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_CLASS]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_ELSE]          = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_FALSE]         = {literal,   NULL,    PREC_NONE       },
  [TOKEN_FOR]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_FUN]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_IF]            = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_NIL]           = {literal,   NULL,    PREC_NONE       },
  [TOKEN_OR]            = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_PRINT]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_RETURN]        = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_SUPER]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_THIS]          = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_TRUE]          = {literal,   NULL,    PREC_NONE       },
  [TOKEN_VAR]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_WHILE]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_ERROR]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_EOF]           = {NULL,      NULL,    PREC_NONE       },
/*+---------------------+-----------+--------+-----------------+*/
};

static ParseRule* get_rule(TokenType type) { return &rules[type]; }

bool compile(const char* source, Chunk* chunk) {
  compiling_chunk = chunk;

  init_scanner(source);
  init_parser();

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  end_compiler();

  return !parser.had_error;
}
