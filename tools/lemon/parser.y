%include {
#include "compiler/parser.h"
#include "compiler/tokens.h"        
#include "compiler/ast/type/builders/enum.h"
#include "compiler/ast/type/builders/kvt.h"
#include "compiler/ast/type/builders/sarray.h"
#include "compiler/ast/query/builders/create.h"
#include "compiler/ast/query/builders/delete.h"
#include "compiler/ast/type/types.h"      
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

%token  IDENTIFIER STRING.

%token  INTEGER FLOAT.

%token  SEMICOLON LEFT_BRACKET RIGHT_BRACKET LEFT_BRACE RIGHT_BRACE LEFT_PAREN RIGHT_PAREN COMMA.


/* -------------------------------------------------------------------
   Non-terminal return types
------------------------------------------------------------------- */
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

%type query              {query}
%type create_decl        {query}
%type delete_decl        {query}

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
   Top-level Type Parsing
------------------------------------------------------------------- */
main ::= in. 
in ::= .
in ::= in state SEMICOLON.

state ::= query(A). {
    res->result = A;
    res->ready = true;
}

query(A) ::= delete_decl(B). {
    A = B;
}

query(A) ::= create_decl(B). {
    A = B;
}

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

