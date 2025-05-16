#pragma once

#include "ast/type/types.h"               // type
#include "compiler/stack_parser/common.h" // stackp_result
#include "compiler/tokens.h"              // token

#include "ast/type/builders/kvt.h" // kvt_builder

typedef struct
{
  enum
  {
    KVTP_WAITING_FOR_LB,
    KVTP_WAITING_FOR_IDENT,
    KVTP_WAITING_FOR_TYPE,
    KVTP_WAITING_FOR_COMMA_OR_RIGHT,
    KVTP_DONE,
  } state;

  kvt_builder builder;

} kvt_parser;

kvt_parser kvp_create (lalloc *alloc);

stackp_result kvp_build_union (union_t *dest, kvt_parser *sb, lalloc *destination, error *e);
stackp_result kvp_build_struct (struct_t *dest, kvt_parser *sb, lalloc *destination, error *e);

stackp_result kvp_accept_token (kvt_parser *eb, token t, error *e);
stackp_result kvp_accept_type (kvt_parser *eb, type type, error *e);
