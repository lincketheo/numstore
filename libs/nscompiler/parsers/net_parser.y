%include {
#include <numstore/compiler/parser_more.h>
#include <numstore/compiler/tokens.h>
#include <numstore/compiler/network/insert.h>
}

%name net_parser
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
%type net_insert {struct net_insert_query}

/* -------------------------------------------------------------------
   Error Handling
------------------------------------------------------------------- */
%syntax_error {
  error_causef(
    res->e,
    ERR_SYNTAX,
    "Net parser syntax error at token: %s at index: %d",
    tt_tostr(yyminor.type), res->tnum);
}

%parse_failure {
  error_causef(
    res->e,
    ERR_SYNTAX,
    "Net parser error at index: %d\n", res->tnum);
}

/* -------------------------------------------------------------------
Example:
insert varname 8080 100 50

Grammar:
NET_INSERT -> insert IDENTIFIER INTEGER INTEGER INTEGER SEMICOLON
------------------------------------------------------------------- */
statement ::= net_insert(I) SEMICOLON.
{
    // For now just store basic info
    // TODO: Implement actual net insert handling
    res->done = true;
}

net_insert(A) ::= INSERT IDENTIFIER(V) INTEGER(P) INTEGER(S) INTEGER(L). {
    A.vname = V.str;
    A.port = P.integer;
    A.start = S.integer;
    A.len = L.integer;
}
