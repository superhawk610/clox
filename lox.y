/**
 * Yacc grammar for the Lox language, adapted from
 * https://craftinginterpreters.com/appendix-i.html
 */

%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <ctype.h>

  extern int yylex();
  void yyerror(const char* err);
%}

%define api.value.type {double}
%token NUM

%% // -- grammar rules --

input:
  %empty
| input line
;

line:
  '\n'
| exp '\n'   { printf("\t%.10g\n", $1); }
| error '\n' { yyerrok;                 }

program : declaration* EOF ; // not exactly sure how yacc handles EOF

// -- declarations --

declaration : class_decl
            | fun_decl
            | var_decl
            | statement ;

class_decl : TOKEN_CLASS IDENTIFIER ( '<' IDENTIFIER )?
             '{' function* '}' ;

fun_decl : TOKEN_FUN function ;

var_decl : TOKEN_VAR IDENTIFIER ( '=' expression )? ';' ;

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

assignment : ( call '.' )? IDENTIFIER '=' assignment
           | logic_or ;

logic_or : logic_and ( TOKEN_OR logic_and )* ;

logic_and : equality ( TOKEN_AND equality )* ;

equality : comparison ( ( TOKEN_BANG_EQUAL | TOKEN_EQUAL_EQUAL ) comparison )* ;

comparison : term ( ( '>' | TOKEN_GREATER_EQUAL | '<' | TOKEN_LESS_EQUAL ) term )* ;

term : factor ( ( '-' | '+' ) factor )* ;

factor : unary ( ( '/' | '*' ) unary )* ;

unary : ( '!' | '-' ) unary | call ;

call : primary ( '(' arguments? ')' | '.' IDENTIFIER )* ;

primary : TOKEN_TRUE | TOKEN_FALSE | TOKEN_NIL | TOKEN_THIS
        | NUMBER | STRING | IDENTIFIER | '(' expression ')'
        | TOKEN_SUPER '.' IDENTIFIER ;

// -- utilities --

function : IDENTIFIER '(' parameters? ')' block ;

parameters : IDENTIFIER ( ',' IDENTIFIER )* ;

arguments : expression ( ',' expression )* ;

%% // -- epilogue --

void yyerror(const char* err) {
  fprintf(stderr, "%s\n", err);
}

int main() {
  return yyparse();
}
