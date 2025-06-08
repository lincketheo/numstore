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

  u32 working_start;   // Starting position to "free" [builder]
  kvt_builder builder; // Builds [result]

  lalloc *destination; // Destination to allocate [result]
  struct               // Final struct or union built on last token
  {
    enum
    {
      KVT_STRUCT,
      KVT_UNION,
    } type;

    union
    {
      union_t un_res;
      struct_t st_res;
    };
  } result;

} kvt_parser;

kvt_parser kvp_create (lalloc *working, lalloc *destination, type_t type);
stackp_result kvp_accept_token (kvt_parser *eb, token t, error *e);
stackp_result kvp_accept_type (kvt_parser *eb, type type, error *e);
