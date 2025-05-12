#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/tokens.h"
#include "mm/lalloc.h"
#include "intf/types.h"
#include "type/builders/sarray.h"

typedef struct type_parser_s type_parser;

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

stackp_result sap_create (type_parser *dest, lalloc *alloc);

stackp_result sap_build (type_parser *eb);

sb_feed_t sap_expect_next (const type_parser *tb, token t);

stackp_result sap_accept_token (type_parser *eb, token t);

stackp_result sap_accept_type (type_parser *eb, type type);
