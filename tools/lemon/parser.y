%include {
#include "compiler/parser.h"
#include "compiler/tokens.h"        

#include "numstore/type/builders/enum.h"
#include "numstore/type/builders/kvt.h"
#include "numstore/type/builders/sarray.h"

#include "numstore/query/builders/create.h"
#include "numstore/query/builders/delete.h"
#include "numstore/query/builders/insert.h"

#include "compiler/value/builders/object.h"
#include "compiler/value/builders/array.h"

#include "numstore/type/types.h"      
#include "compiler/expression.h"

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

%type query              {query}
%type create_decl        {query}
%type delete_decl        {query}
%type insert_decl        {query}

%type type               {type}
%type type_spec          {type}

%type enum_decl          {enum_builder}
%type enum_items         {enum_builder}

%type struct_decl        {kvt_builder}
%type struct_items       {kvt_builder}

%type union_decl         {kvt_builder}
%type union_items        {kvt_builder}

%type sarray_decl        {sarray_builder}
%type sarray_dims        {sarray_builder}

%type value              {value}

%type object_decl        {object_builder}
%type object_items       {object_builder}

%type array_decl         {array_builder}
%type array_items        {array_builder}

%type expression         {expr*}
%type equality           {expr*}
%type comparison         {expr*}
%type term               {expr*}
%type factor             {expr*}
%type unary              {expr*}
%type primary            {expr*}

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


/* Top Level */
main ::= in. 
in ::= .
in ::= in state SEMICOLON.

state ::= query(A). {
    res->result = A;
    res->ready = true;
}

/* Query Top Level */
query(A) ::= delete_decl(B). {
    A = B;
}

query(A) ::= create_decl(B). {
    A = B;
}

query(A) ::= insert_decl(B). {
    A = B;
}

/* Type top level */
type_spec(A) ::= type(B). { A = B; }

type(A) ::= enum_decl(B). {
    A = (type){ .type = T_ENUM };
    if (enb_build(&A.en, &B, res->e)) break;
}

type(A) ::= struct_decl(B). {
    A = (type){ .type = T_STRUCT };
    if (kvb_struct_t_build(&A.st, &B, res->e)) break;
}

type(A) ::= union_decl(B). {
    A = (type){ .type = T_UNION };
    if (kvb_union_t_build(&A.un, &B, res->e)) break;
}

type(A) ::= sarray_decl(B). {
    A = (type){ .type = T_SARRAY };
    if (sab_build(&A.sa, &B, res->e)) break;
}

type(A) ::= PRIM(B). {
  A = (type){ 
    .type = T_PRIM,
    .p = B.prim,
  };
}

/* Value Top Level */
value(A) ::= object_decl(B). {
  A = (value){ .type = VT_OBJECT };
  if(objb_build(&A.obj, &B, res->e)) break;
}

value(A) ::= array_decl(B). {
  A = (value){ .type = VT_ARRAY };
  if(arb_build(&A.arr, &B, res->e)) break;
}

/* Low Hanging fruit */
value(A) ::= STRING(B). {
  A = value_string_create(B.str); 
}

value(A) ::= INTEGER(B). {
  A = value_number_create(B.integer); 
}

value(A) ::= FLOAT(B). {
  A = value_number_create(B.floating); 
}

/* TODO - complex */

value(A) ::= TRUE. {
  A = value_true_create(); 
}

value(A) ::= FALSE. {
  A = value_false_create(); 
}

/* -------------------------------------------------------------------
   Enum: enum { A, B, C }
------------------------------------------------------------------- */
enum_decl(A) ::= ENUM LEFT_BRACE enum_items(B) RIGHT_BRACE. { A = B; }

enum_items(A) ::= IDENTIFIER(tok).
{
    A = enb_create(res->work, res->dest);
    if (enb_accept_key(&A, tok.str, res->e)) break;
}

enum_items(A) ::= enum_items(B) COMMA IDENTIFIER(tok).   /* more */
{
    A = B;
    if (enb_accept_key(&A, tok.str, res->e)) break;
}

/* -------------------------------------------------------------------
   Struct: struct { field type, … }   (fixed first-field rule)
------------------------------------------------------------------- */
struct_decl(A) ::= STRUCT LEFT_BRACE struct_items(B) RIGHT_BRACE. { A = B; }

/* 1st field  ——  create builder here */
struct_items(A) ::= IDENTIFIER(tok) type_spec(t).          
{
    A = kvb_create(res->work, res->dest);                 /* <-- was “A = B;” (undefined) */
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

/* subsequent fields —— propagate builder */
struct_items(A) ::= struct_items(B) COMMA IDENTIFIER(tok) type_spec(t).
{
    A = B;
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

/* -------------------------------------------------------------------
   Union: union { tag type, … }
------------------------------------------------------------------- */
union_decl(A) ::= UNION LEFT_BRACE union_items(B) RIGHT_BRACE. { A = B; }

/* 1st variant */
union_items(A) ::= IDENTIFIER(tok) type_spec(t).
{
    A = kvb_create(res->work, res->dest);
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

/* subsequent variants */
union_items(A) ::= union_items(B) COMMA IDENTIFIER(tok) type_spec(t).
{
    A = B;
    if (kvb_accept_key (&A, tok.str, res->e)) break;
    if (kvb_accept_type(&A, t,       res->e)) break;
}

/* -------------------------------------------------------------------
   Sarray: [NUM][NUM]....[NUM] type 

SARRAY  -> SAD TYPE 
SAD     -> [NUM] | [NUM] SAD
------------------------------------------------------------------- */
sarray_decl(A) ::= sarray_dims(B) type(C). {  
  if (sab_accept_type(&B, C, res->e)) break;
  A = B; 
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(B) RIGHT_BRACKET. 
{ 
  A = sab_create(res->work, res->dest);
  if (sab_accept_dim(&A, B.integer, res->e)) break;
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(C) RIGHT_BRACKET sarray_dims(B). 
{
  if (sab_accept_dim(&B, C.integer, res->e)) break;
  A = B;
}

/* -------------------------------------------------------------------
   CREATE: create IDENT TYPE
------------------------------------------------------------------- */
create_decl(A) ::= CREATE(B) IDENTIFIER(I) type_spec(T). {
    A = B.q;

    // Build the builder
    create_builder tmp = crb_create();

    // Accept
    if (crb_accept_string(&tmp, I.str, res->e)) break;
    if (crb_accept_type(&tmp, T, res->e)) break;

    // Build the query
    if (crb_build(A.create, &tmp, res->e)) break; 
}

/* -------------------------------------------------------------------
   DELETE: delete IDENT 
------------------------------------------------------------------- */
delete_decl(A) ::= DELETE(B) IDENTIFIER(I). {
    A = B.q;

    // Build the builder
    delete_builder tmp = dltb_create();

    // Accept
    if (dltb_accept_string(&tmp, I.str, res->e)) break;

    // Build the query
    if (dltb_build(A.delete, &tmp, res->e)) break; 
}

/* -------------------------------------------------------------------
   INSERT: insert IDENT START VALUE
------------------------------------------------------------------- */
insert_decl(A) ::= INSERT(B) IDENTIFIER(I) INTEGER(N) expression(T). {
    A = B.q;

    // Build the builder
    insert_builder tmp = inb_create();

    // Evaluate expression 
    value v;
    if(expr_evaluate(&v, T, res->work, res->e)) break;

    // Accept
    if (inb_accept_string(&tmp, I.str, res->e)) break;
    if (inb_accept_value(&tmp, v, res->e)) break;
    if (inb_accept_start(&tmp, N.integer, res->e)) break;

    // Build the query
    if (inb_build(A.insert, &tmp, res->e)) break; 
}

/* -------------------------------------------------------------------
   Object: { ident : value, ... }
------------------------------------------------------------------- */
object_decl(A) ::= LEFT_BRACE object_items(B) RIGHT_BRACE. { A = B; }

/* 1st variant */
object_items(A) ::= IDENTIFIER(tok) COLON expression(T).
{
    // Evaluate expression 
    value v;
    if(expr_evaluate(&v, T, res->work, res->e)) break;

    A = objb_create(res->work, res->dest);
    if (objb_accept_string(&A, tok.str, res->e)) break;
    if (objb_accept_value(&A, v,       res->e)) break;
}

/* subsequent variants */
object_items(A) ::= object_items(B) COMMA IDENTIFIER(tok) COLON expression(T).
{
    // Evaluate expression 
    value v;
    if(expr_evaluate(&v, T, res->work, res->e)) break;

    A = B;
    if (objb_accept_string(&A, tok.str, res->e)) break;
    if (objb_accept_value(&A, v,       res->e)) break;
}

/* -------------------------------------------------------------------
   Array: [ value, ... ]
------------------------------------------------------------------- */
array_decl(A) ::= LEFT_BRACKET array_items(B) RIGHT_BRACKET. {  A = B; }

/* 1st variant */ 
array_items(A) ::= expression(T). 
{ 
  // Evaluate expression 
  value v;
  if(expr_evaluate(&v, T, res->work, res->e)) break;

  A = arb_create(res->work, res->dest);
  if (arb_accept_value(&A, v, res->e)) break;
}

/* subsequent variants */
array_items(A) ::= array_items(B) COMMA expression(T).
{
  // Evaluate expression 
  value v;
  if(expr_evaluate(&v, T, res->work, res->e)) break;

  A = B;
  if (arb_accept_value(&A, v, res->e)) break;
}

/* -------------------------------------------------------------------
   Expression 

     expression  → equality
     equality    → comparison ( ( "!=" | "==" ) comparison )*
     comparison  → term       ( ( ">"  | ">=" | "<" | "<=" ) term )*
     term        → factor     ( ( "+"  | "-"  ) factor )*
     factor      → unary      ( ( "/"  | "*"  ) unary  )*
     unary       → ( "!" | "-" ) unary | primary
     primary     → value | "(" expression ")"
------------------------------------------------------------------- */

expression(A) ::= equality(B).
{
  A = B;
}

/* ───── equality (==, !=) ───── */
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

/* ───── comparison (>, >=, <, <=) ───── */
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

/* ───── term (+, -) ───── */
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

/* ───── factor (*, /) ───── */
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

/* ───── unary (!, -) ───── */
unary(A) ::= BANG unary(B). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }
  /* fixed argument order: op first, expr second */
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

/* ───── primary (value | "(" expression ")") ───── */
primary(A) ::= value(B). 
{
  A = lmalloc(res->work, 1, sizeof *A);
  if (A == NULL) {
    error_causef(res->e, ERR_NOMEM, "Failed to allocate expression");
    break;
  }
  *A = create_value_expr(B);
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
