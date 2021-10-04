/**
 * Yacc grammar for the Lox language, adapted from
 * https://craftinginterpreters.com/appendix-i.html
 */

%{
  #include <stdarg.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <ctype.h>

  extern int yylineno;
  extern int yylex();
%}

%code provides {
  void yyerror(const char* msg, ...);
}

%locations

%define api.value.type union

%define api.token.prefix {TOKEN_}

%define parse.error verbose

%token

  // TODO: add remaining tokens

  EOL    "end-of-line"
  EOF 0  "end-of-file"
;

%token <double> NUMBER "number"

%token <char*> STRING "string"
%printer { fprintf(yyo, "\"%s\"", $$); } <char*>
%destructor { free($$); } <char*>

%token <char*> IDENTIFIER "identifier"

%% // -- grammar rules --

input : %empty
      | input line ;

line  : eol
      | exps eol
      | error eol { yyerrok; }

eol   : EOF
      | EOL ;

exps  : exp
      | exps exp ;

exp   : NUMBER
      | STRING
      | IDENTIFIER ;

// TODO: Continue porting grammar
// (see https://github.com/akimd/bison/tree/master/examples/c/reccalc)

/*
program : declaration* TOKEN_EOF ;

// -- declarations --

declaration : class_decl
            | fun_decl
            | var_decl
            | statement ;

class_decl : TOKEN_CLASS TOKEN_IDENTIFIER ( '<' TOKEN_IDENTIFIER )?
             '{' function* '}' ;

fun_decl : TOKEN_FUN function ;

var_decl : TOKEN_VAR TOKEN_IDENTIFIER ( '=' expression )? ';' ;

// -- statements --

statement : expr_stmt
          | for_stmt
          | if_stmt
          | print_stmt
          | return_stmt
          | while_stmt
          | block ;

expr_stmt : expression ';' ;

for_stmt : TOKEN_FOR '(' ( var_decl | expr_stmt | ';' )
                         expression? ';'
                         expression? ')' statement ;

if_stmt : TOKEN_IF '(' expression ')' statement
          ( TOKEN_ELSE statement )? ;

print_stmt : TOKEN_PRINT expression ';' ;

return_stmt : TOKEN_RETURN expression? ';' ;

while_stmt : TOKEN_WHILE '(' expression ')' statement ;

block : '{' declaration* '}' ;

// -- expressions --

expression : assignment ;

assignment : ( call '.' )? TOKEN_IDENTIFIER '=' assignment
           | logic_or ;

logic_or : logic_and ( TOKEN_OR logic_and )* ;

logic_and : equality ( TOKEN_AND equality )* ;

equality : comparison ( ( TOKEN_BANG_EQUAL | TOKEN_EQUAL_EQUAL ) comparison )* ;

comparison : term ( ( '>' | TOKEN_GREATER_EQUAL | '<' | TOKEN_LESS_EQUAL ) term )* ;

term : factor ( ( '-' | '+' ) factor )* ;

factor : unary ( ( '/' | '*' ) unary )* ;

unary : ( '!' | '-' ) unary | call ;

call : primary ( '(' arguments? ')' | '.' TOKEN_IDENTIFIER )* ;

primary : TOKEN_TRUE | TOKEN_FALSE | TOKEN_NIL | TOKEN_THIS
        | NUMBER | STRING | TOKEN_IDENTIFIER | '(' expression ')'
        | TOKEN_SUPER '.' TOKEN_IDENTIFIER ;

// -- utilities --

function : TOKEN_IDENTIFIER '(' parameters? ')' block ;

parameters : TOKEN_IDENTIFIER ( ',' TOKEN_IDENTIFIER )* ;

arguments : expression ( ',' expression )* ;
*/

%% // -- epilogue --

void yyerror(const char* msg, ...) {
  // struct YYLTYPE {
  //   int first_line;
  //   int first_column;
  //   int last_line;
  //   int last_column;
  // } yyloc;

  fprintf(stderr, "error (line %d:%d)", yylineno, yylloc.first_column);

  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);

  fputc('\n', stderr);
}

int main() {
  return yyparse();
}
