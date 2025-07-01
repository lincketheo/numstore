#pragma once

#include "compiler/value/values/array.h"
#include "compiler/value/values/object.h"
#include "core/ds/llist.h"
#include "core/ds/strings.h"
#include "core/intf/types.h"
#include "core/mm/lalloc.h"

typedef enum
{
  // Composite
  VT_OBJECT,
  VT_ARRAY,

  // Simple
  VT_STRING,
  VT_NUMBER,
  VT_COMPLEX,
  VT_TRUE,
  VT_FALSE,
} value_t;

const char *value_t_tostr (value_t);

typedef struct value_s value;
struct value_s
{
  value_t type;

  union
  {
    object obj;
    array arr;
    string str;
    f128 number;
    cf128 cplx;
  };
};

static inline value
value_string_create (string str)
{
  return (value){
    .str = str,
    .type = VT_STRING,
  };
}

static inline value
value_number_create (f128 number)
{
  return (value){
    .type = VT_NUMBER,
    .number = number,
  };
}

static inline value
value_complex_create (cf128 cplx)
{
  return (value){
    .type = VT_COMPLEX,
    .cplx = cplx,
  };
}

static inline value
value_true_create (void)
{
  return (value){
    .type = VT_TRUE,
  };
}

static inline value
value_false_create (void)
{
  return (value){
    .type = VT_FALSE,
  };
}

bool value_equal (const value *left, const value *right);

err_t value_plus_value (value *dest, const value *right, lalloc *alloc, error *e);
err_t value_minus_value (value *dest, const value *right, error *e);
err_t value_star_value (value *dest, const value *right, error *e);
err_t value_slash_value (value *dest, const value *right, error *e);
err_t value_equal_equal_value (value *dest, const value *right, error *e);
err_t value_bang_equal_value (value *dest, const value *right, error *e);
err_t value_greater_value (value *dest, const value *right, error *e);
err_t value_greater_equal_value (value *dest, const value *right, error *e);
err_t value_less_value (value *dest, const value *right, error *e);
err_t value_less_equal_value (value *dest, const value *right, error *e);

err_t value_minus (value *dest, error *e);
void value_bang (value *dest);

void i_log_value (value *v);
