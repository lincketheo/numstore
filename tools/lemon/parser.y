%include {
#include "compiler/parser.h"
#include "compiler/tokens.h"        
#include "ast/type/builders/enum.h"
#include "ast/type/builders/kvt.h"
#include "ast/type/builders/sarray.h"
#include "ast/query/builders/create.h"
#include "ast/query/builders/delete.h"
#include "ast/type/types.h"      
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wtype-limits"
}

%name lemon_parse
%token_prefix "TT_"
%token_type   {token}

/* Allow access to allocator + error object */
%extra_argument { parser_ctxt *ctxt }

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
    ctxt->e, 
    ERR_SYNTAX, 
    "Syntax error at token: %s at index: %d", 
    tt_tostr(yyminor.type), ctxt->tnum);
}

%parse_failure {
  error_causef(
    ctxt->e, 
    ERR_SYNTAX, 
    "Parser error at index: %d\n", ctxt->tnum);
}


/* -------------------------------------------------------------------
   Top-level Type Parsing
------------------------------------------------------------------- */
program ::= query(A). {
  i_log_query(A); 
}

query(A) ::= delete_decl(B). {
    A = B;
}

query(A) ::= create_decl(B). {
    A = B;
}

type(A) ::= enum_decl(B). {
    type tmp = { .type = T_ENUM };
    err_t_wrap(enb_build(&tmp.en, &B, ctxt->e), ctxt->e);
    A = tmp;
}

type_spec(A) ::= type(B). { A = B; }

type(A) ::= struct_decl(B). {
    type tmp = { .type = T_STRUCT };
    err_t_wrap(kvb_struct_t_build(&tmp.st, &B, ctxt->e), ctxt->e);
    A = tmp;
}

type(A) ::= union_decl(B). {
    type tmp = { .type = T_UNION };
    err_t_wrap(kvb_union_t_build(&tmp.un, &B, ctxt->e), ctxt->e);
    A = tmp;
}

type(A) ::= sarray_decl(B). {
    type tmp = { .type = T_SARRAY };
    err_t_wrap(sab_build(&tmp.sa, &B, ctxt->e), ctxt->e);
    A = tmp;
}

type(A) ::= prim(B). {
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
    A = enb_create(ctxt->work, ctxt->dest);
    err_t_wrap(enb_accept_key(&A, tok.str, ctxt->e), ctxt->e);
}

enum_items(A) ::= enum_items(B) COMMA IDENTIFIER(tok).   /* more */
{
    A = B;
    err_t_wrap(enb_accept_key(&A, tok.str, ctxt->e), ctxt->e);
}

/* -------------------------------------------------------------------
   Struct: struct { field type, … }   (fixed first-field rule)
------------------------------------------------------------------- */
struct_decl(A) ::= STRUCT LEFT_BRACE struct_items(B) RIGHT_BRACE. { A = B; }

/* 1st field  ——  create builder here */
struct_items(A) ::= IDENTIFIER(tok) type_spec(t).          
{
    A = kvb_create(ctxt->work, ctxt->dest);                 /* <-- was “A = B;” (undefined) */
    err_t_wrap(kvb_accept_key (&A, tok.str, ctxt->e), ctxt->e);
    err_t_wrap(kvb_accept_type(&A, t,       ctxt->e), ctxt->e);
}

/* subsequent fields —— propagate builder */
struct_items(A) ::= struct_items(B) COMMA IDENTIFIER(tok) type_spec(t).
{
    A = B;
    err_t_wrap(kvb_accept_key (&A, tok.str, ctxt->e), ctxt->e);
    err_t_wrap(kvb_accept_type(&A, t,       ctxt->e), ctxt->e);
}

/* -------------------------------------------------------------------
   Union: union { tag type, … }
------------------------------------------------------------------- */
union_decl(A) ::= UNION LEFT_BRACE union_items(B) RIGHT_BRACE. { A = B; }

/* 1st variant */
union_items(A) ::= IDENTIFIER(tok) type_spec(t).
{
    A = kvb_create(ctxt->work, ctxt->dest);
    err_t_wrap(kvb_accept_key (&A, tok.str, ctxt->e), ctxt->e);
    err_t_wrap(kvb_accept_type(&A, t,       ctxt->e), ctxt->e);
}

/* subsequent variants */
union_items(A) ::= union_items(B) COMMA IDENTIFIER(tok) type_spec(t).
{
    A = B;
    err_t_wrap(kvb_accept_key (&A, tok.str, ctxt->e), ctxt->e);
    err_t_wrap(kvb_accept_type(&A, t,       ctxt->e), ctxt->e);
}

/* -------------------------------------------------------------------
   Sarray: [NUM][NUM]....[NUM] type 

SARRAY  -> SAD TYPE 
SAD     -> [NUM] | [NUM] SAD
------------------------------------------------------------------- */
sarray_decl(A) ::= sarray_dims(B) type(C). {  
  err_t_wrap(sab_accept_type(&B, C, ctxt->e), ctxt->e);
  A = B; 
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(B) RIGHT_BRACKET. 
{ 
  A = sab_create(ctxt->work, ctxt->dest);
  err_t_wrap(sab_accept_dim(&A, B.integer, ctxt->e), ctxt->e);
}

sarray_dims(A) ::= LEFT_BRACKET INTEGER(C) RIGHT_BRACKET sarray_dims(B). 
{
  err_t_wrap(sab_accept_dim(&B, C.integer, ctxt->e), ctxt->e);
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
    err_t_wrap(crb_accept_string(&tmp, I.str, ctxt->e), ctxt->e);
    err_t_wrap(crb_accept_type(&tmp, T, ctxt->e), ctxt->e);

    // Build the query
    err_t_wrap(crb_build(A.create, &tmp, ctxt->e), ctxt->e); 
}

/* -------------------------------------------------------------------
   DELETE: delete IDENT 
------------------------------------------------------------------- */
delete_decl(A) ::= DELETE(B) IDENTIFIER(I). {
    A = B.q;

    // Build the builder
    delete_builder tmp = dltb_create();

    // Accept
    err_t_wrap(dltb_accept_string(&tmp, I.str, ctxt->e), ctxt->e);

    // Build the query
    err_t_wrap(dltb_build(A.delete, &tmp, ctxt->e), ctxt->e); 
}

/* -------------------------------------------------------------------
   Prim
------------------------------------------------------------------- */
prim ::= PRIM.
