#pragma once

#include "query/queries/create.h"
#include "variables/vmem_hashmap.h"

typedef struct
{
  vmem_hashmap *hm;
} var_create;

typedef struct
{
  vmem_hashmap *hm;
} vcreate_params;

var_create var_create_create (vcreate_params params);

err_t var_create_create_var (var_create *v, create_query cargs);
