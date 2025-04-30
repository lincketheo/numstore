#pragma once

#include "compiler/ast_builders/queries/append_builder.h"
#include "compiler/ast_builders/queries/create_builder.h"
#include "compiler/ast_builders/queries/delete_builder.h"
#include "compiler/ast_builders/queries/insert_builder.h"
#include "compiler/ast_builders/queries/read_builder.h"
#include "compiler/ast_builders/queries/take_builder.h"
#include "compiler/ast_builders/queries/update_builder.h"
#include "query/query.h"

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
parse_result qb_accept_token (query_builder *eb, token t);
parse_result qb_accept_type (query_builder *eb, type type);
parse_result qb_build (query_builder *eb);
