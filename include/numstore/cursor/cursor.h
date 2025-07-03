#pragma once

#include "compiler/ast/query.h" // create_query
#include "core/errors/error.h"  // error

#include "numstore/paging/pager.h" // pager

typedef struct cursor_s cursor;

cursor *cursor_open (pager *p, error *e);
void cursor_close (cursor *c);

// CREATE
err_t cursor_create (cursor *c, create_query *q, error *e);

// DELETE
err_t cursor_delete (cursor *c, delete_query *q, error *e);

// INSERT
err_t cursor_insert (cursor *c, insert_query *q, error *e);
