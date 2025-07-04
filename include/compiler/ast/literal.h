#pragma once

#include "core/ds/llist.h"
#include "core/ds/strings.h"
#include "core/intf/types.h"
#include "core/mm/lalloc.h"

typedef struct literal_s literal;

///////////////////////////
/// Object and array literal types
typedef struct
{
  string *keys;
  literal *literals;
  u32 len;
} object;

typedef struct
{
  literal *literals;
  u32 len;
} array;

i32 object_t_snprintf (char *str, u32 size, const object *st);
bool object_equal (const object *left, const object *right);
err_t object_plus (object *dest, const object *right, lalloc *alloc, error *e);

bool array_equal (const array *left, const array *right);
err_t array_plus (array *dest, const array *right, lalloc *alloc, error *e);

///////////////////////////
/// Literal
typedef enum
{
  // Composite
  LT_OBJECT,
  LT_ARRAY,

  // Simple
  LT_STRING,
  LT_INTEGER,
  LT_DECIMAL,
  LT_COMPLEX,
  LT_BOOL,
} literal_t;

const char *literal_t_tostr (literal_t);
bool literal_equal (const literal *left, const literal *right);

typedef struct literal_s literal;
struct literal_s
{
  literal_t type;

  union
  {
    bool bl;
    object obj;
    array arr;
    string str;
    i32 integer;
    f32 decimal;
    cf64 cplx;
  };
};

///////////////////////////
/// Object / Array builders

typedef struct
{
  string key;
  literal v;
  llnode link;
} object_llnode;

typedef struct
{
  literal v;
  llnode link;
} array_llnode;

typedef struct
{
  llnode *head;

  u16 klen;
  u16 tlen;

  lalloc *work;
  lalloc *dest;
} object_builder;

typedef struct
{
  llnode *head;

  lalloc *work;
  lalloc *dest;
} array_builder;

object_builder objb_create (lalloc *work, lalloc *dest);
err_t objb_accept_string (object_builder *o, const string key, error *e);
err_t objb_accept_literal (object_builder *o, literal v, error *e);
err_t objb_build (object *dest, object_builder *b, error *e);

array_builder arb_create (lalloc *work, lalloc *dest);
err_t arb_accept_literal (array_builder *o, literal v, error *e);
err_t arb_build (array *dest, array_builder *b, error *e);

///////////////////////////
/// Simple constructors for the other types

static inline literal
literal_string_create (string str)
{
  return (literal){
    .str = str,
    .type = LT_STRING,
  };
}

static inline literal
literal_integer_create (i64 integer)
{
  return (literal){
    .type = LT_INTEGER,
    .integer = integer,
  };
}

static inline literal
literal_decimal_create (f128 decimal)
{
  return (literal){
    .type = LT_DECIMAL,
    .decimal = decimal,
  };
}

static inline literal
literal_complex_create (cf128 cplx)
{
  return (literal){
    .type = LT_COMPLEX,
    .cplx = cplx,
  };
}

static inline literal
literal_bool_create (bool bl)
{
  return (literal){
    .type = LT_BOOL,
    .bl = bl,
  };
}

///////////////////////////
/// Expression reductions

// dest = dest + right
err_t literal_plus_literal (literal *dest, const literal *right, lalloc *alloc, error *e);

// dest = dest - right
err_t literal_minus_literal (literal *dest, const literal *right, error *e);

// dest = dest * right
err_t literal_star_literal (literal *dest, const literal *right, error *e);

// dest = dest / right
err_t literal_slash_literal (literal *dest, const literal *right, error *e);

// dest = dest == right
err_t literal_equal_equal_literal (literal *dest, const literal *right, error *e);

// dest = dest != right
err_t literal_bang_equal_literal (literal *dest, const literal *right, error *e);

// dest = dest > right
err_t literal_greater_literal (literal *dest, const literal *right, error *e);

// dest = dest >= right
err_t literal_greater_equal_literal (literal *dest, const literal *right, error *e);

// dest = dest < right
err_t literal_less_literal (literal *dest, const literal *right, error *e);

// dest = dest <= right
err_t literal_less_equal_literal (literal *dest, const literal *right, error *e);

// dest = dest ^ right
err_t literal_caret_literal (literal *dest, const literal *right, error *e);

// dest = dest % right
err_t literal_mod_literal (literal *dest, const literal *right, error *e);

// dest = dest | right
err_t literal_pipe_literal (literal *dest, const literal *right, error *e);

// dest = dest || right
void literal_pipe_pipe_literal (literal *dest, const literal *right);

// dest = dest & right
err_t literal_ampersand_literal (literal *dest, const literal *right, error *e);

// dest = dest && right
void literal_ampersand_ampersand_literal (literal *dest, const literal *right);

// dest = ~dest
err_t literal_not (literal *dest, error *e);

// dest = -dest
err_t literal_minus (literal *dest, error *e);

// dest = !dest
void literal_bang (literal *dest);

void i_log_literal (literal *v);
