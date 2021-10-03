/**
 * Lex vocabulary for the Lox language.
 */

%{
  #include <stdio.h>

  #include "build/lox.h"
%}

%option noyywrap

ALPHA       [a-zA-Z_]
DIGIT       [0-9]
WHITESPACE  [ \n\t\r]

%%

{WHITESPACE}+  { /* ignore whitespace */ }

{DIGIT}+(\.{DIGIT}+)?      { printf("  (number)=> %s\n", yytext); }
\"[^\"]+\"                 { printf("  (string)=> %s\n", yytext); }
{ALPHA}({ALPHA}|{DIGIT})*  { printf("   (ident)=> %s\n", yytext); }
