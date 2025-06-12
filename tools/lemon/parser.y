%include {
#include "compiler/parser.h"
#include "compiler/tokens.h"        
#include "ast/type/builders/enum.h"
#include "ast/type/builders/kvt.h"
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
   Tokens
------------------------------------------------------------------- */


/* -------------------------------------------------------------------
   Non-terminal return types
------------------------------------------------------------------- */
%type type               {type}
%type type_spec          {type}
%type enum_decl          {enum_builder}
%type struct_decl        {kvt_builder}
%type union_decl         {kvt_builder}
%type enum_items_opt     {enum_builder}
%type enum_items         {enum_builder}
%type struct_items_opt   {kvt_builder}
%type struct_items       {kvt_builder}
%type union_items_opt    {kvt_builder}
%type union_items        {kvt_builder}

/* -------------------------------------------------------------------
   Top-level Type Parsing
------------------------------------------------------------------- */
program ::= type.

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
/* -------------------------------------------------------------------
   Enum: enum { A, B, C }
------------------------------------------------------------------- */
enum_decl(A) ::= ENUM LEFT_BRACE enum_items_opt(B) RIGHT_BRACE. { A = B; }

enum_items_opt(A) ::= .                         /* empty enum */
{ A = enb_create(ctxt->work, ctxt->dest); }

enum_items_opt(A) ::= enum_items(B).            { A = B; }

enum_items(A) ::= IDENTIFIER(tok).              /* 1st enumerator */
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
struct_decl(A) ::= STRUCT LEFT_BRACE struct_items_opt(B) RIGHT_BRACE. { A = B; }

struct_items_opt(A) ::= .                        /* empty struct */
{ A = kvb_create(ctxt->work, ctxt->dest); }

struct_items_opt(A) ::= struct_items(B).         { A = B; }

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
union_decl(A) ::= UNION LEFT_BRACE union_items_opt(B) RIGHT_BRACE. { A = B; }

union_items_opt(A) ::= .                         /* empty union */
{ A = kvb_create(ctxt->work, ctxt->dest); }

union_items_opt(A) ::= union_items(B).           { A = B; }

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

%code {
# pragma GCC diagnostic pop
}
 
