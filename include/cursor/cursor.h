#pragma once

#include "ast/query/queries/create.h" // create_query
#include "errors/error.h"             // error
#include "paging/pager.h"             // pager

typedef struct cursor_s cursor;

cursor *cursor_open (pager *p, error *e);
void cursor_close (cursor *c);

err_t cursor_create_var (cursor *c, create_query *create_q, error *e);
