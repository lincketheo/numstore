#pragma once

#include "ds/cbuffer.h" // cbuffer

typedef struct
{
  cbuffer *queries_input;
} vm;

typedef struct
{
  cbuffer *queries_input;
} vm_params;

vm vm_create (vm_params args);

void vm_execute (vm *v);
