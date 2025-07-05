#pragma once

#include "core/ds/cbuffer.h"
#include "core/errors/error.h" // error

#include "numstore/paging/pager.h" // pager
#include "numstore/type/types/prim.h"

typedef struct cursor_s cursor;

cursor *cursor_open (pager *p, error *e);
void cursor_close (cursor *c);

// CREATE
err_t cursor_create (
    cursor *c,
    string vname,
    type type,
    error *e);

// DELETE
err_t cursor_delete (
    cursor *c,
    string vname,
    error *e);

// INSERT
err_t cursor_insert (
    cursor *c,
    string vname,    // The variable to insert data into
    b_size start,    // The starting index
    b_size len,      // The length of data (in [vname]'s type)
    cbuffer *source, // The source to feed data into
    error *e);

// READ
err_t cursor_read (
    cursor *c,
    string vname,
    b_size from,
    b_size n,
    b_size stride,
    cbuffer *dest,
    error *e);

// WRITE
err_t cursor_write (
    cursor *c,
    string vname,
    b_size from,
    b_size n,
    b_size stride,
    cbuffer *dest,
    error *e);

err_t cursor_execute (cursor *c, error *e);
bool cursor_idle (cursor *c);
