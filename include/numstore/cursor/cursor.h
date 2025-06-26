#pragma once

#include "core/errors/error.h" // error

#include "numstore/paging/pager.h"         // pager
#include "numstore/query/queries/create.h" // create_query
#include "numstore/query/queries/delete.h" // TODO
#include "numstore/query/queries/insert.h"

typedef struct cursor_s cursor;

cursor *cursor_open (pager *p, error *e);
void cursor_close (cursor *c);

/**
 * Primary routines
 */

// CREATE
err_t cursor_create (cursor *c, create_query *q, error *e);

// DELETE
err_t cursor_delete (cursor *c, delete_query *q, error *e);

// INSERT
err_t cursor_insert (cursor *c, insert_query *q, error *e);
