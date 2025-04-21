#pragma once

#include "dev/assert.h"
#include "paging.h"
#include "sds.h"
#include "typing.h"

////////////////////////////// A Single Variable
typedef struct
{
  u64 page0;
  type *type;
} variable;

DEFINE_DBG_ASSERT_H (variable, variable, v);

////////////////////////////// A Subset of a Variable
typedef struct
{
  variable v;
  type_subset *type_subset;
} variable_subset;

DEFINE_DBG_ASSERT_H (variable_subset, variable_subset, v);

////////////////////////////// A Hash Map of Variables
typedef struct
{
  pager *pgr;
} var_retriver;

DEFINE_DBG_ASSERT_H (var_retriver, var_retriver, v);

err_t vr_get (
    var_retriver *v,
    variable *dest,
    int *exists,
    const string vname);

err_t vr_set (
    var_retriver *v,
    variable *dest,
    const string vname,
    const type *type);
