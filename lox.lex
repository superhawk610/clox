/**
 * Lex vocabulary for the Lox language.
 */

%{
  #include <stdio.h>

  #include "build/lox.h"
%}

%option noyywrap

%%

[ \n\t\r]+ { /* ignore whitespace */ }

[0-9]+     { printf("  => %s\n", yytext); }
[a-zA-Z]+  { printf("  => %s\n", yytext); }
