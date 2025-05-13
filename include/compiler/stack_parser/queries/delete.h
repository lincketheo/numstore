#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token
#include "type/types.h"                   // type

#include "query/builders/delete.h" // delete_builder
#include "query/queries/delete.h"  // delete_query

typedef struct
{
  enum
  {
    DB_WAITING_FOR_VNAME,
    DB_DONE,
  } state;

  delete_builder builder;

} delete_parser;

delete_parser dltp_create (void);

stackp_result dltp_build (delete_query *dest, delete_parser *eb, error *e);

stackp_result dltp_accept_token (delete_parser *eb, token t, error *e);
