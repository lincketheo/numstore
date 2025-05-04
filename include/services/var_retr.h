#pragma once

#include "vhash_map.h"

typedef struct
{
  const vhash_map *hm;
} var_retr;

typedef struct
{
  vhash_map *hm;
} vrtr_params;

var_retr var_retr_create (vrtr_params params);

err_t var_retr_get_var (var_retr *v, vmeta *dest, const string vname);
