#pragma once

#include "ast/query/queries/create.h" // create_query
#include "ast/query/queries/delete.h"
#include "errors/error.h" // error
#include "paging/pager.h" // pager

typedef struct cursor_s cursor;

cursor *cursor_open (pager *p, error *e);
void cursor_close (cursor *c);

/**
 * Primary routines
 */

// CREATE
err_t cursor_create_var (cursor *c, create_query *q, error *e);

// DELETE
err_t cursor_delete_var (cursor *c, delete_query *q, error *e);
