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
 * qspace provider provides query space. Areas of memory
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
} qspce_prvdr;

qspce_prvdr *qspce_prvdr_create (error *e);

err_t qspce_prvdr_get (
    qspce_prvdr *q,
    query *dest,
    query_t type,
    error *e);

void qspce_prvdr_release (qspce_prvdr *q, query *qu);

void qspce_prvdr_free (qspce_prvdr *q);
