/*
 * cool.flex  (PA2 - COOL Scanner)
 *
 * Organização:
 *  1) Espaços e contagem de linhas
 *  2) Comentários: linha (-- ...) e bloco aninhado ((* ... *))
 *  3) Operadores multi-caractere: => <= <-
 *  4) Palavras-chave (case-insensitive) + booleanos
 *  5) Identificadores e inteiros
 *  6) Strings (estado STRING) com escapes e erros
 *  7) Tokens de 1 caractere válidos
 *  8) Catch-all: qualquer caractere inválido -> ERROR com msg (1 char)
 */

%{
#include <cool-parse.h>
#include <stringtab.h>
#include <utilities.h>
#include <cstring>   /* strdup */

/* O compilador assume estes identificadores */
#define yylval cool_yylval
#define yylex  cool_yylex

#define MAX_STR_CONST 1025
#define YY_NO_UNPUT

extern FILE *fin;

/* Faz o flex ler do FILE* fin */
#undef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
  if ((result = fread((char*)buf, sizeof(char), max_size, fin)) < 0) \
    YY_FATAL_ERROR("read() in flex scanner failed");

extern int curr_lineno;
extern YYSTYPE cool_yylval;

/* ---- auxiliares ---- */
static int comment_depth = 0;

static int string_too_long = 0;
char string_buf[MAX_STR_CONST];
char *string_buf_ptr;

%}

%option noyywrap

/* Condições iniciais (modos) */
%x COMMENT
%x STRING

/* ---------- macros (regex) ---------- */
WS              [ \t\r\f\v]+
DIGIT           [0-9]
INT             {DIGIT}+
IDCHAR          [A-Za-z0-9_]
TYPEID          [A-Z]{IDCHAR}*
OBJECTID        [a-z]{IDCHAR}*

%%





{WS}                    { /* ignora */ }
\n                      { curr_lineno++; }





"--".*                  { /* ignora até \n (o \n é contado pela regra acima) */ }

"*)"                    {
                          cool_yylval.error_msg = strdup("Unmatched *)");
                          return ERROR;
                        }

"(*"                    {
                          comment_depth = 1;
                          BEGIN(COMMENT);
                        }

<COMMENT>"(*"           { comment_depth++; }
<COMMENT>"*)"           {
                          comment_depth--;
                          if (comment_depth == 0) BEGIN(INITIAL);
                        }
<COMMENT>\n             { curr_lineno++; }
<COMMENT>.              { /* consome */ }
<COMMENT><<EOF>>        {
                          cool_yylval.error_msg = strdup("EOF in comment");
                          BEGIN(INITIAL);
                          return ERROR;
                        }





"=>"                    { return DARROW; }
"<="                    { return LE; }
"<-"                    { return ASSIGN; }





[cC][lL][aA][sS][sS]             { return CLASS; }
[iI][fF]                         { return IF; }
[tT][hH][eE][nN]                 { return THEN; }
[eE][lL][sS][eE]                 { return ELSE; }
[fF][iI]                         { return FI; }
[iI][nN]                         { return IN; }
[iI][nN][hH][eE][rR][iI][tT][sS] { return INHERITS; }
[lL][eE][tT]                     { return LET; }
[wW][hH][iI][lL][eE]             { return WHILE; }
[lL][oO][oO][pP]                 { return LOOP; }
[pP][oO][oO][lL]                 { return POOL; }
[cC][aA][sS][eE]                 { return CASE; }
[eE][sS][aA][cC]                 { return ESAC; }
[oO][fF]                         { return OF; }
[nN][eE][wW]                     { return NEW; }
[iI][sS][vV][oO][iI][dD]         { return ISVOID; }
[nN][oO][tT]                     { return NOT; }


t[rR][uU][eE]                    { cool_yylval.boolean = 1; return BOOL_CONST; }
f[aA][lL][sS][eE]                { cool_yylval.boolean = 0; return BOOL_CONST; }





{INT}                   { cool_yylval.symbol = inttable.add_string(yytext); return INT_CONST; }
{TYPEID}                { cool_yylval.symbol = idtable.add_string(yytext);  return TYPEID; }
{OBJECTID}              { cool_yylval.symbol = idtable.add_string(yytext);  return OBJECTID; }




\"                      {
                          BEGIN(STRING);
                          string_buf_ptr = string_buf;
                          string_too_long = 0;
                        }

<STRING>\"              {
                          BEGIN(INITIAL);
                          *string_buf_ptr = '\0';

                          if (string_too_long) {
                            cool_yylval.error_msg = strdup("String constant too long");
                            return ERROR;
                          }

                          cool_yylval.symbol = stringtable.add_string(string_buf);
                          return STR_CONST;
                        }
<STRING>\n              {
                          BEGIN(INITIAL);
                          curr_lineno++;
                          cool_yylval.error_msg = strdup("Unterminated string constant");
                          return ERROR;
                        }

<STRING><<EOF>>         {
                          BEGIN(INITIAL);
                          cool_yylval.error_msg = strdup("EOF in string constant");
                          return ERROR;
                        }
<STRING>\0              {
                          BEGIN(INITIAL);
                          cool_yylval.error_msg = strdup("String contains null character");
                          return ERROR;
                        }
<STRING>\\n             { if (!string_too_long) { *string_buf_ptr++ = '\n'; if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1; } }
<STRING>\\t             { if (!string_too_long) { *string_buf_ptr++ = '\t'; if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1; } }
<STRING>\\b             { if (!string_too_long) { *string_buf_ptr++ = '\b'; if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1; } }
<STRING>\\f             { if (!string_too_long) { *string_buf_ptr++ = '\f'; if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1; } }
<STRING>\\\\            { if (!string_too_long) { *string_buf_ptr++ = '\\'; if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1; } }
<STRING>\\\"            { if (!string_too_long) { *string_buf_ptr++ = '\"'; if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1; } }
<STRING>\\\n            { curr_lineno++; }

<STRING>\\(.)           {
                          if (!string_too_long) {
                            *string_buf_ptr++ = yytext[1];
                            if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1;
                          }
                        }
<STRING>.               {
                          if (!string_too_long) {
                            *string_buf_ptr++ = yytext[0];
                            if (string_buf_ptr - string_buf >= MAX_STR_CONST-1) string_too_long = 1;
                          }
                        }





[+\-*/=<.\~,@;:(){}]     { return yytext[0]; }




.                       {
                          char buf[2];
                          buf[0] = yytext[0];
                          buf[1] = '\0';
                          cool_yylval.error_msg = strdup(buf);
                          return ERROR;
                        }

%%




#undef yylex
extern "C" int yylex(void);
extern "C" int yylex(void) { return cool_yylex(); }