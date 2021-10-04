/**
 * Lex vocabulary for the Lox language.
 */

%{
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>

  #include "build/lox.h"

  #define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
%}

%option noyywrap yylineno

ALPHA       [a-zA-Z_]
DIGIT       [0-9]
WHITESPACE  [ \t\r]

%%

{WHITESPACE}+               continue; // ignore whitespace

"\n"                        return TOKEN_EOL;

{DIGIT}+(\.{DIGIT}+)?       {
                              printf("  (number)=> %s\n", yytext);

                              double n = strtod(yytext, NULL);
                              yylval.TOKEN_NUMBER = n;
                              return TOKEN_NUMBER;
                            }

\"[^\"]+\"                  {
                              printf("  (string)=> %s\n", yytext);

                              size_t len = strlen(yytext) - 1; // add 1 for null terminator, then
                              char* str = (char*) malloc(len); // subtract 2 to remove quotation marks
                              memcpy(str, yytext + 1, len - 1);
                              str[len - 1] = '\0';
                              yylval.TOKEN_STRING = str;
                              return TOKEN_STRING;
                            }

{ALPHA}({ALPHA}|{DIGIT})*   {
                              printf("   (ident)=> %s\n", yytext);

                              size_t len = strlen(yytext) + 1; // add 1 for null terminator
                              char* str = (char*) malloc(len);
                              memcpy(str, yytext, len - 1);
                              str[len - 1] = '\0';
                              yylval.TOKEN_IDENTIFIER = str;
                              return TOKEN_IDENTIFIER;
                            }

.                           yyerror("syntax error, invalid character: %c", yytext[0]);

<<EOF>>                     return TOKEN_EOF;

%%
