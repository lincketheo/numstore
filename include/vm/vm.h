#pragma once

#include "ds/cbuffer.h"

typedef struct
{
  cbuffer *queries_input;
} vm;

typedef struct
{
  cbuffer *queries_input;
} vm_params;

void vm_create (vm *dest, vm_params args);

void vm_execute (vm *v);
