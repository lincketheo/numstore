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
  VT_IDENT,
  VT_NUMBER,
  VT_COMPLEX,
  VT_TRUE,
  VT_FALSE,
} value_t;

typedef struct value_s value;
struct value_s
{
  value_t type;

  union
  {
    object obj;
    array arr;
    string str;
    string ident;
    f128 number;
    cf128 complex;
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
value_ident_create (string ident)
{
  return (value){
    .ident = ident,
    .type = VT_IDENT,
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
value_complex_create (cf128 complex)
{
  return (value){
    .type = VT_COMPLEX,
    .complex = complex,
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
