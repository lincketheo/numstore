#pragma once

#include "ast/query/query.h" // query
#include "ds/cbuffer.h"
#include "errors/error.h" // err_t
#include "paging/pager.h" // pager

typedef struct vm_s vm;

vm *vm_open (pager *p, error *e);
void vm_close (vm *vm);
err_t vm_execute_one_query (vm *v, query *q, error *e);
