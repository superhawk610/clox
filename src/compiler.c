#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chunk.h"
#include "compiler.h"
#include "scanner.h"
#include "value.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

#define DEPTH_UNITIALIZED -1
#define UNRESOLVED_LOCAL -1

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

// Local variables are stored on the stack and resolved
// at runtime by walking and comparing by `name`.
//
// Locals are associated with the _scope_ in which they
// were defined, marked by their `depth`. The top-level
// scope has a depth of 0, and each block created within
// that increases the depth by 1.
typedef struct {
  Token name;
  int depth;
} Local;

typedef enum {
  TYPE_SCRIPT, // the top-level script is compiled as a "function"
  TYPE_FUNCTION,
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing; // used to track the compiler that kicked off
                              // this compiler (it's "parent"); for top-level
                              // compilation, this will be NULL

  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int local_count;
  int scope_depth;
} Compiler;

Parser parser;
Compiler* current = NULL;

static Chunk* current_chunk() { return &current->function->chunk; }

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

// emit a jump instruction, and return the bytecode offset of its
// target operand to be patched once we know how far to jump
static int emit_jump(uint8_t instr) {
  emit_byte(instr);
  emit_byte(0xff);
  emit_byte(0xff);
  return current_chunk()->len - 2;
}

// emit a loop instruction that will jump back to `offset` in
// the bytecode (adjusted by 2 to account for OP_LOOP's operand)
static void emit_loop(int loop_start) {
  emit_byte(OP_LOOP);

  int offset = (int) current_chunk()->len - loop_start + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

//
//     OP_JUMP
//       JUMP_TARGET_HI  <-- offset points here
//       JUMP_TARGET_LO
//                       <-- ip will be here after reading OP_JUMP
//     ... (other ops)
//
//     (you are here)    <-- current_chunk()->len
//
static void patch_jump(int offset) {
  // current_chunk()->len - offset gives number of bytes to jump over,
  // then subtract 2 to account for the JUMP_TARGET_* bytes (they will
  // already have been read by the time the VM is performing the jump)
  int jump = current_chunk()->len - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  current_chunk()->code[offset] = (jump >> 8) & 0xff;
  current_chunk()->code[offset + 1] = jump & 0xff;
}

static void emit_constant(Value val) {
  uint16_t constant = add_constant(current_chunk(), val);
  emit_constant_op(constant, OP_CONST, OP_CONST_LONG);
}

static void emit_return() {
  emit_byte(OP_NIL); // functions implicitly return nil (if no value is specified)
  emit_byte(OP_RETURN);
}

static void init_compiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = new_function();
  compiler->type = type;
  compiler->local_count = 0;
  compiler->scope_depth = 0;
  current = compiler;

  // we've just parsed the function's name (that's what kicks off compilation
  // with a fresh compiler instance), use it as this compiler's name
  if (type != TYPE_SCRIPT) {
    current->function->name = copy_string(parser.previous.start, parser.previous.len);
  }

  // the compiler claims the first stack slot for its own usage
  Local* local = &current->locals[current->local_count++];
  local->depth = 0;
  local->name.start = "";
  local->name.len = 0;
}

static ObjFunction* end_compiler() {
  emit_return();

  ObjFunction* func = current->function;

#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    disasm_chunk(current_chunk(), func->name == NULL
                                  ? "<script>" : func->name->chars);
  }
#endif

  current = current->enclosing; // shift compilation back to the parent

  return func;
}

static void begin_scope() {
  current->scope_depth++;
}

static void end_scope() {
  current->scope_depth--;

  // discard all scoped locals from the stack
  while (current->local_count > 0 &&
         current->locals[current->local_count - 1].depth > current->scope_depth) {
    emit_byte(OP_POP);
    current->local_count--;
  }
}

static void parse_precedence(Precedence prec);
static ParseRule* get_rule(TokenType type);
static void expression();
static void statement();
static void declaration();
static void var_declaration();

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
  Value str = OBJ_VAL((Obj*) copy_string(parser.previous.start + 1,
                                         parser.previous.len - 2));

  uint16_t constant;
  bool found = value_array_find_index(&current_chunk()->constants, str, &constant);
  if (!found) constant = add_constant(current_chunk(), str);

  emit_constant_op(constant, OP_CONST, OP_CONST_LONG);
}

static uint16_t identifier_constant(Token* name) {
  Value str = OBJ_VAL((Obj*) copy_string(name->start, name->len));

  uint16_t existing;
  bool found = value_array_find_index(&current_chunk()->constants, str, &existing);
  return found ? existing : add_constant(current_chunk(), str);
}

static uint8_t argument_list() { // parse 0 or more argument expressions for a function
  uint8_t argc = 0;              // call, placing them sequentially on the stack

  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argc == 255) error("Can't have more than 255 arguments.");
      argc++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
  return argc;
}

static bool identifiers_equal(Token* a, Token* b) {
  if (a->len != b->len) return false;
  return memcmp(a->start, b->start, a->len) == 0;
}

static int resolve_local(Compiler* compiler, Token* name) {
  for (int i = compiler->local_count - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiers_equal(name, &local->name)) {
      if (local->depth == DEPTH_UNITIALIZED) {
        error("Can't read local variable in its own initializer.");
      }

      return i;
    }
  }

  return UNRESOLVED_LOCAL;
}

static void add_local(Token name) {
  // we could dynamically allocate additional storage for locals, but
  // for now let's just error out (256 should be a reasonable default)
  if (current->local_count == UINT8_COUNT) {
    error("Too many local variables in function.");
    return;
  }

  Local* local = &current->locals[current->local_count++];
  local->name = name;
  local->depth = DEPTH_UNITIALIZED; // locals are marked as uninitialized until
                                    // their initializer expression has been parsed,
                                    // so that this sort of thing is marked as invalid
                                    //
                                    //     var a = "outer";
                                    //     {
                                    //       var a = a;
                                    //     }
                                    //
}

static void declare_variable() {
  // no need to declare globals, as they're embedded in the bytecode
  if (current->scope_depth == 0) return;

  Token* name = &parser.previous;

  for (int i = current->local_count - 1; i >= 0; i--) {
    Local* local = &current->locals[i];             // starting at the top of the stack, walk
    if (local->depth != DEPTH_UNITIALIZED &&        // backwards and make sure this is the first
        local->depth < current->scope_depth) break; // time we've seen this declaration in this block

    if (identifiers_equal(name, &local->name)) {
      error("Cannot redeclare block-scoped variable.");
    }
  }

  add_local(*name);
}

static void named_local(uint8_t arg, bool can_assign) {
  if (can_assign && match(TOKEN_EQUAL)) { // if followed by `=` (i.e. a = 1),
    expression();                         // it's variable assignment...
    emit_bytes(OP_SET_LOCAL, arg);
  } else {                                // ...otherwise, it's just variable access
    emit_bytes(OP_GET_LOCAL, arg);
  }
}

static void named_global(uint16_t arg, bool can_assign) {
  if (can_assign && match(TOKEN_EQUAL)) {
    expression();
    emit_constant_op(arg, OP_SET_GLOBAL, OP_SET_GLOBAL_LONG);
  } else {
    emit_constant_op(arg, OP_GET_GLOBAL, OP_GET_GLOBAL_LONG);
  }
}

static void named_variable(Token name, bool can_assign) {
  int local_arg = resolve_local(current, &name);
  if (local_arg != UNRESOLVED_LOCAL) {
    named_local((uint8_t) local_arg, can_assign);
  } else {
    uint16_t arg = identifier_constant(&name);
    named_global(arg, can_assign);
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
  // binary operations are left-associative; in other words, we want
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

static void call(bool can_assign) {
  uint8_t argc = argument_list();
  emit_bytes(OP_CALL, argc);
}

// and expressions will generate this control flow
//
//      <left operand expression>
//
//  ┌── OP_JUMP_IF_FALSE
//  |   OP_POP
//  |
//  |   <right operand expression>
//  |
//  └-> resume execution...
//
static void and_(bool can_assign) {
  int end_jump = emit_jump(OP_JUMP_IF_FALSE);

  emit_byte(OP_POP);
  parse_precedence(PREC_AND);

  patch_jump(end_jump);
}

// or expressions will generate this control flow
//
//         <left operand expression>
//
//     ┌── OP_JUMP_IF_FALSE
//   ┌─┼── OP_JUMP
//   | └-> OP_POP
//   |
//   |     <right operand expression>
//   |
//   └──-> resume execution...
//
static void or_(bool can_assign) {
  int else_jump = emit_jump(OP_JUMP_IF_FALSE);
  int end_jump = emit_jump(OP_JUMP);

  patch_jump(else_jump);
  emit_byte(OP_POP);

  parse_precedence(PREC_OR);
  patch_jump(end_jump);
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

  declare_variable();                     // if the variable is local, no need to
  if (current->scope_depth > 0) return 0; // store it in the constants table, just
                                          // return a dummy value

  return identifier_constant(&parser.previous);
}

static inline void mark_initialized() {
  if (current->scope_depth == 0) return; // top-level functions are stored as globals,
                                         // so this doesn't need to do anything

  current->locals[current->local_count - 1].depth = current->scope_depth;
}

static void define_variable(uint16_t global) {
  // if defining a local, it's already on top of the stack
  if (current->scope_depth > 0) {
    mark_initialized();
    return;
  }

  emit_constant_op(global, OP_DEF_GLOBAL, OP_DEF_GLOBAL_LONG);
}

static void expression() {
  parse_precedence(PREC_ASSIGNMENT); // parse with the lowest precedence, so
                                     // all higher-precedence expressions are
                                     // parsed as well
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void function(FunctionType type) {
  Compiler compiler;

  init_compiler(&compiler, type);
  begin_scope();

  consume(TOKEN_LEFT_PAREN, "Expected '(' after function name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        error_at_current("Can't have more than 255 function parameters.");
      }

      uint16_t constant = parse_variable("Expected parameter name.");
      define_variable(constant);
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after function parameters.");

  consume(TOKEN_LEFT_BRACE, "Expected '{' before function body.");
  block();

  /* consume(TOKEN_RIGHT_BRACE, "..."); // may be omitted */

  ObjFunction* func = end_compiler();
  emit_constant(OBJ_VAL((Obj*) func));

  /* end_scope(); // may also be omitted (the compiler has finished) */
}

static void expression_statement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expected ';' after expression.");
  emit_byte(OP_POP); // evaluate the expression, then discard the result
}

// if statements will generate this control flow
//
//         <condition expression>
//
//     ┌── OP_JUMP_IF_FALSE
//     |   OP_POP
//     |
//     |   <then branch statement>
//     |
//   ┌─┼── OP_JUMP
//   | └-> OP_POP
//   |
//   |     <else branch statement>
//   |
//   └──-> resume execution...
//
static void if_statement() {
  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

  int then_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP); // pop the condition expression from the stack
  statement();

  int else_jump = emit_jump(OP_JUMP);

  patch_jump(then_jump);
  emit_byte(OP_POP);

  if (match(TOKEN_ELSE)) statement();
  patch_jump(else_jump);
}

// while loops will generate this control flow
//
//      <condition expression> <-┐
//                               |
//  ┌── OP_JUMP_IF_FALSE         |
//  |   OP_POP                   |
//  |                            |
//  |   <body statement>         |
//  |                            |
//  |   OP_LOOP ─────────────────┘
//  └-> OP_POP
//
//      resume execution...
//
static void while_statement() {
  int loop_start = (int) current_chunk()->len;

  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expected ')' after condition.");

  int exit_jump = emit_jump(OP_JUMP_IF_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_loop(loop_start);

  patch_jump(exit_jump);
  emit_byte(OP_POP);
}

// for loops will generate this control flow
//
//         <initializer clause>
//
//         <condition expression> <-─┐
//                                   |
//     ┌── OP_JUMP_IF_FALSE          |    * if the condition expression is omitted,
//     |   OP_POP                    |      the OP_JUMP_IF_FALSE will be as well and
//   ┌─┼── OP_JUMP                   |      the loop will effectively be infinite
//   | |                             |
//   | |   <increment expression> <-─┼─┐  * if the increment clause is omitted,
//   | |                             | |    the OP_POP and OP_LOOP ops will be
//   | |   OP_POP                    | |    as well, and the body loop will jump
//   | |   OP_LOOP ──────────────────┘ |    directly to the condition expression
//   | |                               |
//   └─┼-> <body statement>            |
//     |                               |
//     |   OP_LOOP ────────────────────┘
//     └-> OP_POP
//
//         resume execution...
//
static void for_statement() {
  begin_scope();

  consume(TOKEN_LEFT_PAREN, "Expected '(' after 'for'.");

  // (optionally) parse initializer clause
  if (match(TOKEN_SEMICOLON)) { /* do nothing */ }
  else if (match(TOKEN_VAR))  var_declaration();
  else                        expression_statement();

  int loop_start = (int) current_chunk()->len;
  int exit_jump = -1;

  // (optionally) parse condition clause
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

    exit_jump = emit_jump(OP_JUMP_IF_FALSE);
    emit_byte(OP_POP);
  }

  // (optionally) parse increment clause
  if (!match(TOKEN_RIGHT_PAREN)) {
    int body_jump = emit_jump(OP_JUMP);
    int increment_start = current_chunk()->len;

    expression();
    emit_byte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expected ')' after for clauses.");

    emit_loop(loop_start);
    loop_start = increment_start;
    patch_jump(body_jump);
  }

  statement();
  emit_loop(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  end_scope();
}

// TODO: Add support for `switch`, `break`, and `continue`
// (see https://craftinginterpreters.com/jumping-back-and-forth.html#challenges)

static void print_statement() {
  expression(); // evaluate the `print` statement's operand, placing it on the stack
  consume(TOKEN_SEMICOLON, "Expected ';' after value.");
  emit_byte(OP_PRINT);
}

static void return_statement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  if (match(TOKEN_SEMICOLON)) { // implicit return value (nil)
    emit_return();
  } else {                      // explicit return value (expression)
    expression();
    consume(TOKEN_SEMICOLON, "Expected ';' after return value.");
    emit_byte(OP_RETURN);
  }
}

static void statement() {
  if (match(TOKEN_PRINT))
    print_statement();

  else if (match(TOKEN_IF))
    if_statement();

  else if (match(TOKEN_RETURN))
    return_statement();

  else if (match(TOKEN_WHILE))
    while_statement();

  else if (match(TOKEN_FOR))
    for_statement();

  else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  }

  else
    expression_statement();
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

static void fun_declaration() {
  uint8_t global = parse_variable("Expected function name.");
  mark_initialized(); // we don't need to wait for an initializer expression,
                      // it's OK for functions to recurse since they won't actually
                      // _use_ their own value until runtime

  function(TYPE_FUNCTION);
  define_variable(global);
}

static void var_declaration() {
  uint16_t global = parse_variable("Expected variable name.");

  if (match(TOKEN_EQUAL)) expression();      // read initializer expression, or
  else                    emit_byte(OP_NIL); // default to nil, if none is provided

  consume(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  define_variable(global);
}

static void declaration() {
  if (match(TOKEN_FUN))
    fun_declaration();

  else if (match(TOKEN_VAR))
    var_declaration();

  else
    statement();

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
  [TOKEN_LEFT_PAREN]    = {grouping,  call,    PREC_CALL       },
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
  [TOKEN_AND]           = {NULL,      and_,    PREC_AND        },
  [TOKEN_CLASS]         = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_ELSE]          = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_FALSE]         = {literal,   NULL,    PREC_NONE       },
  [TOKEN_FOR]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_FUN]           = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_IF]            = {NULL,      NULL,    PREC_NONE       },
  [TOKEN_NIL]           = {literal,   NULL,    PREC_NONE       },
  [TOKEN_OR]            = {NULL,      or_,     PREC_OR         },
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

ObjFunction* compile(const char* source) {
  Compiler compiler;

  init_scanner(source);
  init_compiler(&compiler, TYPE_SCRIPT);
  init_parser();

  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* func = end_compiler();
  return parser.had_error ? NULL : func;
}

// ---

#undef DEPTH_UNITIALIZED
#undef UNRESOLVED_LOCAL
