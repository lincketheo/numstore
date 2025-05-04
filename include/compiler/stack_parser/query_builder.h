#pragma once

#include "compiler/stack_parser/common.h"
#include "compiler/stack_parser/queries/append_builder.h"
#include "compiler/stack_parser/queries/create_builder.h"
#include "compiler/stack_parser/queries/delete_builder.h"
#include "compiler/stack_parser/queries/insert_builder.h"
#include "compiler/stack_parser/queries/read_builder.h"
#include "compiler/stack_parser/queries/take_builder.h"
#include "compiler/stack_parser/queries/update_builder.h"
#include "query/query.h"
#include "services/services.h"
#include "services/var_retr.h"

struct query_builder_s
{
  enum
  {
    QB_UNKNOWN,

    QB_CREATE,
    QB_DELETE,
    QB_APPEND,
    QB_INSERT,
    QB_UPDATE,
    QB_READ,
    QB_TAKE,

  } state;

  union
  {
    create_builder cb;
    delete_builder db;
    append_builder ab;
    insert_builder ib;
    update_builder ub;
    read_builder rb;
    take_builder tb;
  };

  query ret;
};

query_builder qb_create (void);
stackp_result qb_build (query_builder *eb);
sb_feed_t qb_expect_next (const query_builder *qb);

// ACCEPT
stackp_result qb_accept_token (query_builder *eb, token t);
stackp_result qb_accept_type (query_builder *eb, type type);
