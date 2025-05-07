#pragma once

#include "services/var_create.h"
#include "services/var_retr.h"
#include "variables/vmem_hashmap.h"

/**
 * A one stop shop for
 * all services
 */
typedef struct
{
  var_create vcreate;
  var_retr vretr;
} services;

typedef struct
{
  vmem_hashmap *vhm;
} services_params;

services services_create (services_params params);
