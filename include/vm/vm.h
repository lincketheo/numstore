#pragma once

#include "ds/cbuffer.h" // cbuffer
#include "stmtctrl.h"

typedef struct
{
  cbuffer *queries_input;

  stmtctrl *ctrl;
} vm;

vm vm_create (cbuffer *queries_input, stmtctrl *ctrl);

void vm_execute (vm *v);
