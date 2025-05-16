#pragma once

#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token
#include "mm/lalloc.h"                    // lalloc

#include "ast/type/builders/sarray.h" // sarray_builder

typedef struct
{

  enum
  {
    SAP_WAITING_FOR_RIGHT,
    SAP_WAITING_FOR_LEFT_OR_TYPE,
    SAP_WAITING_FOR_NUMBER,
    SAP_DONE,
  } state;

  sarray_builder builder;

} sarray_parser;

sarray_parser sap_create (lalloc *alloc);

stackp_result sap_build (
    sarray_t *dest,
    sarray_parser *eb,
    lalloc *destination,
    error *e);

stackp_result sap_accept_token (sarray_parser *eb, token t, error *e);
stackp_result sap_accept_type (sarray_parser *eb, type type, error *e);
