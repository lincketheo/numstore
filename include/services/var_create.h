#pragma once

#include "query/queries/create.h"
#include "vhash_map.h"

typedef struct
{
  vhash_map *hm;
} var_create;

typedef struct
{
  vhash_map *hm;
} vcreate_params;

var_create var_create_create (vcreate_params params);

err_t var_create_create_var (var_create *v, create_query cargs);
