#pragma once

#include "variables/vmem_hashmap.h"

typedef struct
{
  const vmem_hashmap *hm;
} var_retr;

typedef struct
{
  vmem_hashmap *hm;
} vrtr_params;

var_retr var_retr_create (vrtr_params params);

err_t var_retr_get_var (var_retr *v, vmeta *dest, const string vname);
