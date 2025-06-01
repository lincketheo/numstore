#pragma once

#include "ast/query/queries/create.h"
#include "ast/query/queries/delete.h"
#include "ast/query/query.h"

typedef struct
{
  create_query create;
  bool is_used;
} create_wrapper;

typedef struct
{
  delete_query delete;
  bool is_used;
} delete_wrapper;

/**
 * query provider provides query space. Areas of memory
 * to allocate strings onto so that each query has 1 malloc and
 * 1 free.
 *
 * For now while queries are simple, it's just 1 chunk of memory
 * but I'll probably change this in the future so that I'm not
 * so greedy at the start.
 *
 * Flaws (with the 1 block malloc at the start):
 *   1. Set amount of queries of each type. What if a query is more
 *   popular, then there's lots of wasted space.
 */
typedef struct
{
  create_wrapper create[20];
  delete_wrapper delete[20];
} query_provider;

query_provider *query_provider_create (error *e);

err_t query_provider_get (
    query_provider *q,
    query *dest,
    query_t type,
    error *e);

void query_provider_release (query_provider *q, query *qu);

void query_provider_free (query_provider *q);
