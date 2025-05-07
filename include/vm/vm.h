#pragma once

#include "ds/cbuffer.h"
#include "services/services.h"
#include "services/var_create.h"
#include "services/var_retr.h"
#include "variables/vmem_hashmap.h"

typedef struct
{
  cbuffer *queries_input;
  services *services;
} vm;

typedef struct
{
  cbuffer *queries_input;
  services *services;
} vm_params;

void vm_create (vm *dest, vm_params args);

void vm_execute (vm *v);
