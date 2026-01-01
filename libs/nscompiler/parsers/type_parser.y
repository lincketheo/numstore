%include {
#include <numstore/compiler/parser_more.h>
#include <numstore/compiler/tokens.h>
#include <numstore/compiler/shell/insert.h>
}

%name shell_parser
%token_prefix "TT_"
%token_type {struct token}

/* Allow access to allocator + error object */
%extra_argument { parser_result *res }

/* -------------------------------------------------------------------
   Tokens - Ordering
------------------------------------------------------------------- */
//      Arithmetic Operators
%token  PLUS MINUS SLASH STAR.

//      Logical Operators
%token  BANG BANG_EQUAL EQUAL_EQUAL GREATER GREATER_EQUAL LESS LESS_EQUAL.

//      Fancy Operators
%token  NOT CARET PERCENT PIPE AMPERSAND.

//      Other One char tokens
%token  SEMICOLON COLON LEFT_BRACKET RIGHT_BRACKET LEFT_BRACE.
%token  RIGHT_BRACE LEFT_PAREN RIGHT_PAREN COMMA.

//      Other
%token  STRING IDENTIFIER.

// Tokens that start with a number or +/-
%token  INTEGER FLOAT.

//      Literal Operations
%token  INSERT.

//      Bools
%token  TRUE FALSE.

/* -------------------------------------------------------------------
   Non-terminal return types
------------------------------------------------------------------- */
%type shell_insert {struct shell_insert_query}

/* -------------------------------------------------------------------
   Error Handling
------------------------------------------------------------------- */
%syntax_error {
  error_causef(
    res->e,
    ERR_SYNTAX,
    "Shell parser syntax error at token: %s at index: %d",
    tt_tostr(yyminor.type), res->tnum);
}

%parse_failure {
  error_causef(
    res->e,
    ERR_SYNTAX,
    "Shell parser error at index: %d\n", res->tnum);
}

/* -------------------------------------------------------------------
Example:
insert varname 100 50

Grammar:
SHELL_INSERT -> insert IDENTIFIER INTEGER INTEGER SEMICOLON
------------------------------------------------------------------- */
statement ::= shell_insert(I) SEMICOLON.
{
    // For now just store basic info
    // TODO: Implement actual shell insert handling
    res->done = true;
}

shell_insert(A) ::= INSERT IDENTIFIER(V) INTEGER(S) INTEGER(L). {
    A.vname = V.str;
    A.start = S.integer;
    A.len = L.integer;
}
