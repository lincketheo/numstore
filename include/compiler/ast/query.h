#pragma once

#include "compiler/ast/literal.h"
#include "core/mm/lalloc.h"      // TODO
#include "numstore/type/types.h" // type

/////////////////////////////
//// CREATE
typedef struct
{
  string vname;
  type type;
} create_query;

void i_log_create (create_query *q);
bool create_query_equal (const create_query *left, const create_query *right);

typedef struct
{
  string vname;
  type type;
  bool got_type;
  bool got_vname;
} create_builder;

create_builder crb_create (void);
err_t crb_accept_string (create_builder *b, const string vname, error *e);
err_t crb_accept_type (create_builder *b, type t, error *e);
err_t crb_build (create_query *dest, create_builder *b, error *e);

/////////////////////////////
//// DELETE
typedef struct
{
  string vname; // Variable to delete
} delete_query;

void i_log_delete (delete_query *q);
bool delete_query_equal (const delete_query *left, const delete_query *right);

typedef struct
{
  string vname;
  bool got_vname;
} delete_builder;

delete_builder dltb_create (void);
err_t dltb_accept_string (delete_builder *b, const string vname, error *e);
err_t dltb_build (delete_query *dest, delete_builder *b, error *e);

/////////////////////////////
//// INSERT
typedef struct
{
  string vname;
  literal val;
  b_size start;
} insert_query;

void i_log_insert (insert_query *q);
bool insert_query_equal (const insert_query *left, const insert_query *right);

typedef struct
{
  // Has type "Expected"
  string vname; // The variable name
  literal val;  // either ([] Expected) or (Expected)
  b_size start; // Starting index to insert data into
  bool got_vname;
  bool got_val;
  bool got_start;
} insert_builder;

insert_builder inb_create (void);
err_t inb_accept_string (insert_builder *a, const string vname, error *e);
err_t inb_accept_literal (insert_builder *a, literal v, error *e);
err_t inb_accept_start (insert_builder *a, b_size start, error *e);
err_t inb_build (insert_query *dest, insert_builder *b, error *e);

/////////////////////////////
//// QUERY
typedef enum
{
  QT_CREATE,
  QT_DELETE,
  QT_INSERT,
} query_t;

///// QUERY
typedef struct
{
  query_t type;

  union
  {
    create_query create;
    delete_query delete;
    insert_query insert;
  };
} query;

void i_log_query (query *q);
bool query_equal (const query *left, const query *right);
