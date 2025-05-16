#pragma once

#include "ast/query/query.h" // query
#include "errors/error.h"    // err_t
#include "paging/pager.h"    // pager

err_t query_execute (pager *p, query *q, error *e);
