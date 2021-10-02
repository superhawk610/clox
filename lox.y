/**
 * Yacc grammar for the Lox language.
 *
 * OK, so right now it only parses number literals, but
 * bare with me, still learning the ropes :)
 */

%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <math.h>
  #include <ctype.h>

  int yylex();
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
| exp '\n'  { printf("\t%.10g\n", $1); }

exp:
  NUM
;

%% // -- epilogue --

int yylex() {
  int c = getchar();

  // skip whitespace
  while (c == ' ' || c == '\t') c = getchar();

  // process numbers
  if (c == '.' || isdigit(c)) {
    ungetc(c, stdin);
    if (scanf("%lf", &yylval) != 1) abort();
    return NUM;
  }

  else if (c == EOF)
    return YYEOF;

  else
    return c;
}

void yyerror(const char* err) {
  fprintf(stderr, "%s\n", err);
}

int main() {
  return yyparse();
}
