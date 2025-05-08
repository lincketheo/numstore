#pragma once

#include "compiler/stack_parser/queries/append.h"
#include "compiler/stack_parser/queries/create.h"
#include "compiler/stack_parser/queries/delete.h"
#include "compiler/stack_parser/queries/insert.h"
#include "compiler/stack_parser/queries/read.h"
#include "compiler/stack_parser/queries/take.h"
#include "compiler/stack_parser/queries/update.h"

#include "compiler/stack_parser/common.h"
#include "query/query.h"
#include "services/services.h"
#include "services/var_retr.h"

struct query_parser_s
{
  enum
  {
    QP_UNKNOWN,

    QP_CREATE,
    QP_DELETE,
    QP_APPEND,
    QP_INSERT,
    QP_UPDATE,
    QP_READ,
    QP_TAKE,

  } state;

  union
  {
    create_parser cp;
    delete_parser dp;
    append_parser ap;
    insert_parser ip;
    update_parser up;
    read_parser rp;
    take_parser tp;
  };

  query ret;
};

query_parser qryp_create (void);

stackp_result qryp_build (query_parser *eb);

sb_feed_t qryp_expect_next (const query_parser *qb);

stackp_result qryp_accept_token (query_parser *eb, token t);

stackp_result qryp_accept_type (query_parser *eb, type type);
