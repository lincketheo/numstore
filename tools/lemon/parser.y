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
%token  CREATE DELETE APPEND INSERT UPDATE READ TAKE.

%token  BCREATE BDELETE BAPPEND BINSERT BUPDATE BREAD BTAKE.

%token  STRUCT UNION ENUM PRIM.

%token  TRUE FALSE.

%token  IDENTIFIER STRING.

%token  INTEGER FLOAT.

%token  SEMICOLON COLON LEFT_BRACKET RIGHT_BRACKET LEFT_BRACE RIGHT_BRACE LEFT_PAREN RIGHT_PAREN COMMA.

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
%type value_spec         {value}

%type object_decl        {object_builder}
%type object_items       {object_builder}

%type array_decl         {array_builder}
%type array_items        {array_builder}

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
    if (enb_build(&A.en, &B, res->e) != 0) break;
}

type(A) ::= struct_decl(B). {
    A = (type){ .type = T_STRUCT };
    if (kvb_struct_t_build(&A.st, &B, res->e) != 0) break;
}

type(A) ::= union_decl(B). {
    A = (type){ .type = T_UNION };
    if (kvb_union_t_build(&A.un, &B, res->e) != 0) break;
}

type(A) ::= sarray_decl(B). {
    A = (type){ .type = T_SARRAY };
    if (sab_build(&A.sa, &B, res->e) != 0) break;
}

type(A) ::= PRIM(B). {
  A = (type){ 
    .type = T_PRIM,
    .p = B.prim,
  };
}

/* Value Top Level */
value_spec(A) ::= value(B). { A = B; }

value(A) ::= object_decl(B). {
  A = (value){ .type = VT_OBJECT };
  if(objb_build(&A.obj, &B, res->e) != 0) break;
}

value(A) ::= array_decl(B). {
  A = (value){ .type = VT_ARRAY };
  if(arb_build(&A.arr, &B, res->e) != 0) break;
}

/* Low Hanging fruit */
value(A) ::= STRING(B). {
  A = value_string_create(B.str); 
}

value(A) ::= IDENT(B). {
  A = value_ident_create(B.str); 
}

value(A) ::= INTEGER(B). {
  A = value_number_create(B.integer); 
}

value(A) ::= DECIMAL(B). {
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
    if (enb_accept_key(&A, tok.str, res->e) != 0) break;
}

enum_items(A) ::= enum_items(B) COMMA IDENTIFIER(tok).   /* more */
{
    A = B;
    if (enb_accept_key(&A, tok.str, res->e) != 0) break;
}

/* -------------------------------------------------------------------
   Struct: struct { field type, … }   (fixed first-field rule)
------------------------------------------------------------------- */
struct_decl(A) ::= STRUCT LEFT_BRACE struct_items(B) RIGHT_BRACE. { A = B; }

/* 1st field  ——  create builder here */
struct_items(A) ::= IDENTIFIER(tok) type_spec(t).          
{
    A = kvb_create(res->work, res->dest);                 /* <-- was “A = B;” (undefined) */
    if (kvb_accept_key (&A, tok.str, res->e) != 0) break;
    if (kvb_accept_type(&A, t,       res->e) != 0) break;
}

/* subsequent fields —— propagate builder */
struct_items(A) ::= struct_items(B) COMMA IDENTIFIER(tok) type_spec(t).
{
    A = B;
    if (kvb_accept_key (&A, tok.str, res->e) != 0) break;
    if (kvb_accept_type(&A, t,       res->e) != 0) break;
}

/* -------------------------------------------------------------------
   Union: union { tag type, … }
------------------------------------------------------------------- */
union_decl(A) ::= UNION LEFT_BRACE union_items(B) RIGHT_BRACE. { A = B; }

/* 1st variant */
union_items(A) ::= IDENTIFIER(tok) type_spec(t).
{
    A = kvb_create(res->work, res->dest);
    if (kvb_accept_key (&A, tok.str, res->e) != 0) break;
    if (kvb_accept_type(&A, t,       res->e) != 0) break;
}

/* subsequent variants */
union_items(A) ::= union_items(B) COMMA IDENTIFIER(tok) type_spec(t).
{
    A = B;
    if (kvb_accept_key (&A, tok.str, res->e) != 0) break;
    if (kvb_accept_type(&A, t,       res->e) != 0) break;
}

/* -------------------------------------------------------------------
   Sarray: [NUM][NUM]....[NUM] type 

SARRAY  -> SAD TYPE 
SAD     -> [NUM] | [NUM] SAD
------------------------------------------------------------------- */
sarray_decl(A) ::= sarray_dims(B) type(C). {  
  if (sab_accept_type(&B, C, res->e) != 0) break;
  A = B; 
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(B) RIGHT_BRACKET. 
{ 
  A = sab_create(res->work, res->dest);
  if (sab_accept_dim(&A, B.integer, res->e) != 0) break;
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(C) RIGHT_BRACKET sarray_dims(B). 
{
  if (sab_accept_dim(&B, C.integer, res->e) != 0) break;
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
    if (crb_accept_string(&tmp, I.str, res->e) != 0) break;
    if (crb_accept_type(&tmp, T, res->e) != 0) break;

    // Build the query
    if (crb_build(A.create, &tmp, res->e) != 0) break; 
}

/* -------------------------------------------------------------------
   DELETE: delete IDENT 
------------------------------------------------------------------- */
delete_decl(A) ::= DELETE(B) IDENTIFIER(I). {
    A = B.q;

    // Build the builder
    delete_builder tmp = dltb_create();

    // Accept
    if (dltb_accept_string(&tmp, I.str, res->e) != 0) break;

    // Build the query
    if (dltb_build(A.delete, &tmp, res->e) != 0) break; 
}

/* -------------------------------------------------------------------
   INSERT: insert IDENT START VALUE
------------------------------------------------------------------- */
insert_decl(A) ::= INSERT(B) IDENTIFIER(I) INTEGER(N) value_spec(T). {
    A = B.q;

    // Build the builder
    insert_builder tmp = inb_create();

    // Accept
    if (inb_accept_string(&tmp, I.str, res->e) != 0) break;
    if (inb_accept_value(&tmp, T, res->e) != 0) break;
    if (inb_accept_start(&tmp, N.integer, res->e) != 0) break;

    // Build the query
    if (inb_build(A.insert, &tmp, res->e) != 0) break; 
}

/* -------------------------------------------------------------------
   Object: { ident : value, ... }
------------------------------------------------------------------- */
object_decl(A) ::= LEFT_BRACE object_items(B) RIGHT_BRACE. { A = B; }

/* 1st variant */
object_items(A) ::= IDENTIFIER(tok) COLON value_spec(v).
{
    A = objb_create(res->work, res->dest);
    if (objb_accept_string(&A, tok.str, res->e) != 0) break;
    if (objb_accept_value(&A, v,       res->e) != 0) break;
}

/* subsequent variants */
object_items(A) ::= object_items(B) COMMA IDENTIFIER(tok) value_spec(t).
{
    A = B;
    if (objb_accept_string(&A, tok.str, res->e) != 0) break;
    if (objb_accept_value(&A, t,       res->e) != 0) break;
}

/* -------------------------------------------------------------------
   Array: [ value, ... ]
------------------------------------------------------------------- */
array_decl(A) ::= LEFT_BRACKET array_items(B) RIGHT_BRACKET. {  A = B; }

/* 1st variant */ 
array_items(A) ::= value_spec(v). 
{ 
  A = arb_create(res->work, res->dest);
  if (arb_accept_value(&A, v, res->e) != 0) break;
}

/* subsequent variants */
array_items(A) ::= array_items(B) COMMA value_spec(v).
{
  A = B;
  if (arb_accept_value(&A, v, res->e) != 0) break;
}

