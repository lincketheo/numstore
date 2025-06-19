#pragma once

#include "ast/query/query.h" // query
#include "ds/cbuffer.h"
#include "errors/error.h" // err_t
#include "paging/pager.h" // pager

typedef struct vm_s vm;

vm *vm_open (pager *p, cbuffer *input, error *e);
void vm_close (vm *vm);

cbuffer *vm_get_output (vm *v);

void vm_execute (vm *v);
