%include {
#include "compiler/parser.h"
#include "compiler/tokens.h"        

#include "numstore/type/builders/enum.h"
#include "numstore/type/builders/kvt.h"
#include "numstore/type/builders/sarray.h"

#include "compiler/ast/query.h"
#include "compiler/ast/literal.h"
#include "compiler/ast/expression.h"
#include "compiler/ast/statement.h"

#include "numstore/type/types.h"      

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wpedantic"
}

%name lemon_parse
%token_prefix "TT_"
%token_type   {token}

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
%token  CREATE DELETE INSERT.

//      Type literals
%token  STRUCT UNION ENUM PRIM.

//      Bools
%token  TRUE FALSE.

/* -------------------------------------------------------------------
   Non-terminal return types
------------------------------------------------------------------- */

%type query         {query}
%type create        {create_builder}
%type delete        {delete_builder}
%type insert        {insert_builder}

%type type          {type}

%type enum          {enum_builder}
%type enum_items    {enum_builder}

%type struct        {kvt_builder}
%type struct_items  {kvt_builder}

%type union         {kvt_builder}
%type union_items   {kvt_builder}

%type sarray        {sarray_builder}
%type sarray_dims   {sarray_builder}

%type literal       {literal}

%type object        {object_builder}
%type object_items  {object_builder}

%type array         {array_builder}
%type array_items   {array_builder}

%type expression    {expr*}
%type equality      {expr*}
%type comparison    {expr*}
%type term          {expr*}
%type factor        {expr*}
%type unary         {expr*}
%type primary       {expr*}

/* -------------------------------------------------------------------
   Error Handling
------------------------------------------------------------------- */
%syntax_error {
  error_causef(
    res->e, 
    ERR_SYNTAX, 
    "Syntax error at token: %s at index: %d", 
    tt_tostr(yyminor.type), res->tnum);
}

%parse_failure {
  error_causef(
    res->e, 
    ERR_SYNTAX, 
    "Parser error at index: %d\n", res->tnum);
}

/* -------------------------------------------------------------------
STMT -> QUERY;
------------------------------------------------------------------- */
statement ::= query(A) SEMICOLON. 
{
    // Set the resulting query
    res->result->q = A;

    // Notify done
    res->done = true;
}

/* -------------------------------------------------------------------
Primitive
------------------------------------------------------------------- */
type(A) ::= PRIM(B). {
  A = (type){ 
    .type = T_PRIM,
    .p = B.prim,
  };
}


/* -------------------------------------------------------------------
Example:
enum { A, B, C }

Grammar:
ENUM -> enum { ITEMS }
ITEMS -> ident | ITEMS, ident 
------------------------------------------------------------------- */
type(A) ::= enum(B). {
    A = (type){ .type = T_ENUM };
    if (enb_build(&A.en, &B, res->e)) break;
}

enum(A) ::= ENUM LEFT_BRACE enum_items(B) RIGHT_BRACE. { A = B; }

enum_items(A) ::= IDENTIFIER(tok).
{
    A = enb_create(res->work, &res->result->qspace);
    if (enb_accept_key(&A, tok.str, res->e)) break;
}

enum_items(A) ::= enum_items(B) COMMA IDENTIFIER(tok).   
{
    A = B;
    if (enb_accept_key(&A, tok.str, res->e)) break;
}

/* -------------------------------------------------------------------
Example:
struct { a u32, b i32 } 

Grammar:
STRUCT -> struct { ITEMS }
ITEMS -> ident TYPE | ITEMS, ident TYPE
------------------------------------------------------------------- */
type(A) ::= struct(B). {
    A = (type){ .type = T_STRUCT };
    if (kvb_struct_t_build(&A.st, &B, res->e)) break;
}

struct(A) ::= STRUCT LEFT_BRACE struct_items(B) RIGHT_BRACE. { A = B; }

struct_items(A) ::= IDENTIFIER(tok) type(t).          
{
    A = kvb_create(res->work, &res->result->qspace);                 
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

struct_items(A) ::= struct_items(B) COMMA IDENTIFIER(tok) type(t).
{
    A = B;
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

/* -------------------------------------------------------------------
Example:
union { a u32, b i32 }

Grammar:
UNION -> union { ITEMS }
ITEMS -> ident TYPE | ITEMS, ident TYPE
------------------------------------------------------------------- */
type(A) ::= union(B). {
    A = (type){ .type = T_UNION };
    if (kvb_union_t_build(&A.un, &B, res->e)) break;
}

union(A) ::= UNION LEFT_BRACE union_items(B) RIGHT_BRACE. { A = B; }

union_items(A) ::= IDENTIFIER(tok) type(t).
{
    A = kvb_create(res->work, &res->result->qspace);
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

union_items(A) ::= union_items(B) COMMA IDENTIFIER(tok) type(t).
{
    A = B;
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

/* -------------------------------------------------------------------
Example:
[10][5][1] u32

Grammar:
SARRAY  -> SARDIMS TYPE 
SARDIMS -> [integer] | [integer] SARDIMS
------------------------------------------------------------------- */
type(A) ::= sarray(B). {
    A = (type){ .type = T_SARRAY };
    if (sab_build(&A.sa, &B, res->e)) break;
}

sarray(A) ::= sarray_dims(B) type(C). {  
  if (sab_accept_type(&B, C, res->e)) break;
  A = B; 
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(B) RIGHT_BRACKET. 
{ 
  A = sab_create(res->work, &res->result->qspace);
  if (sab_accept_dim(&A, B.integer, res->e)) break;
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(C) RIGHT_BRACKET sarray_dims(B). 
{
  if (sab_accept_dim(&B, C.integer, res->e)) break;
  A = B;
}

/* -------------------------------------------------------------------
Example:
create a u32 

Grammar:
CREATE -> create ident TYPE 
------------------------------------------------------------------- */
query(A) ::= create(B). {
  A = (query){ .type = QT_CREATE };
  if(crb_build(&A.create, &B, res->e)) break;
}

create(A) ::= CREATE IDENTIFIER(I) type(T). {
    // Build the builder
    A = crb_create();

    // Accept
    if (crb_accept_string(&A, I.str, res->e)) break;
    if (crb_accept_type(&A, T, res->e)) break;
}

/* -------------------------------------------------------------------
Example:
delete a

Grammar:
DELETE -> delete ident
------------------------------------------------------------------- */
query(A) ::= delete(B). {
  A = (query){ .type = QT_DELETE };
  if(dltb_build(&A.delete, &B, res->e)) break;
}

delete(A) ::= DELETE IDENTIFIER(I). {
    // Build the builder
    A = dltb_create();

    // Accept
    if (dltb_accept_string(&A, I.str, res->e)) break;
}

/* -------------------------------------------------------------------
Example:
insert a 5 { a: 10, b : 20 }

Grammar:
INSERT -> insert ident integer EXPR
------------------------------------------------------------------- */
query(A) ::= insert(B). {
    A = (query){ .type = QT_INSERT };
    if(inb_build(&A.insert, &B, res->e)) break;
}

insert(A) ::= INSERT IDENTIFIER(I) INTEGER(N) expression(T). {
    // Build the builder
    A = inb_create();

    // For now just evaluate expression - TODO
    literal v;
    if(expr_evaluate(&v, T, res->work, res->e)) break;

    // Accept
    if (inb_accept_string(&A, I.str, res->e)) break;
    if (inb_accept_literal(&A, v, res->e)) break;
    if (inb_accept_start(&A, N.integer, res->e)) break;
}

/* -------------------------------------------------------------------
Example:
{ a : 5, b : { c : 10 } }

Grammar:
OBJECT -> { ITEMS }
ITEMS -> ident : EXPR | ITEMS, ident : EXPR
------------------------------------------------------------------- */
literal(A) ::= object(B). {
  A = (literal){ .type = LT_OBJECT };
  if(objb_build(&A.obj, &B, res->e)) break;
}

object(A) ::= LEFT_BRACE object_items(B) RIGHT_BRACE. { A = B; }

object_items(A) ::= IDENTIFIER(tok) COLON expression(T).
{
    // For now Evaluate expression 
    literal v;
    if(expr_evaluate(&v, T, res->work, res->e)) break;

    A = objb_create(res->work, &res->result->qspace);
    if (objb_accept_string(&A, tok.str, res->e)) break;
    if (objb_accept_literal(&A, v,       res->e)) break;
}

object_items(A) ::= object_items(B) COMMA IDENTIFIER(tok) COLON expression(T).
{
    // For now Evaluate expression 
    literal v;
    if(expr_evaluate(&v, T, res->work, res->e)) break;

    A = B;
    if (objb_accept_string(&A, tok.str, res->e)) break;
    if (objb_accept_literal(&A, v,       res->e)) break;
}

/* -------------------------------------------------------------------
Example:
[ { a: 5 }, [ 1, 2 3], 5 ]

Grammar:
ARRAY -> [ ITEMS ]
ITEMS -> EXPR | ITEMS, EXPR
------------------------------------------------------------------- */
literal(A) ::= array(B). {
  A = (literal){ .type = LT_ARRAY };
  if(arb_build(&A.arr, &B, res->e)) break;
}

array(A) ::= LEFT_BRACKET array_items(B) RIGHT_BRACKET. {  A = B; }

array_items(A) ::= expression(T). 
{ 
  // Evaluate expression 
  literal v;
  if(expr_evaluate(&v, T, res->work, res->e)) break;

  A = arb_create(res->work, &res->result->qspace);
  if (arb_accept_literal(&A, v, res->e)) break;
}

array_items(A) ::= array_items(B) COMMA expression(T).
{
  // Evaluate expression 
  literal v;
  if(expr_evaluate(&v, T, res->work, res->e)) break;

  A = B;
  if (arb_accept_literal(&A, v, res->e)) break;
}

/* -------------------------------------------------------------------
Expression Literals
------------------------------------------------------------------- */
literal(A) ::= STRING(B). {
  A = literal_string_create(B.str); 
}

literal(A) ::= INTEGER(B). {
  A = literal_integer_create(B.integer); 
}

literal(A) ::= FLOAT(B). {
  A = literal_decimal_create(B.floating); 
}

/* TODO - complex */

literal(A) ::= TRUE. {
  A = literal_bool_create(true); 
}

literal(A) ::= FALSE. {
  A = literal_bool_create(false); 
}

/* -------------------------------------------------------------------
EXPRESSION  → EQUALITY
EQUALITY    → COMPARISON ( ( "!=" | "==" ) COMPARISON )*
COMPARISON  → TERM       ( ( ">"  | ">=" | "<" | "<=" ) TERM )*
TERM        → FACTOR     ( ( "+"  | "-"  ) FACTOR )*
FACTOR      → UNARY      ( ( "/"  | "*"  ) UNARY  )*
UNARY       → ( "!" | "-" ) UNARY | PRIMARY
PRIMARY     → literal | "(" EXPRESSION ")"
------------------------------------------------------------------- */
/* EXPR */
expression(A) ::= equality(B).
{
  A = B;
}

/* EQUALITY */
equality(A) ::= comparison(B). 
{
  A = B;
}

equality(A) ::= equality(B) EQUAL_EQUAL comparison(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_EQUAL_EQUAL, C);
}

equality(A) ::= equality(B) BANG_EQUAL comparison(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_BANG_EQUAL, C);
}

/* COMPARISON */
comparison(A) ::= term(B). 
{
  A = B;
}
comparison(A) ::= comparison(B) GREATER term(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_GREATER, C);
}
comparison(A) ::= comparison(B) GREATER_EQUAL term(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_GREATER_EQUAL, C);
}
comparison(A) ::= comparison(B) LESS term(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_LESS, C);
}
comparison(A) ::= comparison(B) LESS_EQUAL term(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_LESS_EQUAL, C);
}

/* TERM */
term(A) ::= factor(B). 
{
  A = B;
}
term(A) ::= term(B) PLUS factor(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }
  *A = create_binary_expr(B, TT_PLUS, C);
}
term(A) ::= term(B) MINUS factor(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_MINUS, C);
}

/* FACTOR */
factor(A) ::= unary(B). 
{
  A = B;
}
factor(A) ::= factor(B) STAR unary(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_STAR, C);
}
factor(A) ::= factor(B) SLASH unary(C). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_binary_expr(B, TT_SLASH, C);
}

/* UNARY */
unary(A) ::= BANG unary(B). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_unary_expr(B, TT_BANG);
}

unary(A) ::= MINUS unary(B). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_unary_expr(B, TT_MINUS);
}

unary(A) ::= primary(B). {
  A = B;
}

/* PRIMARY */
primary(A) ::= literal(B). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_literal_expr(B);
}

primary(A) ::= LEFT_PAREN expression(B) RIGHT_PAREN. 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }

  *A = create_grouping_expr(B);
}
