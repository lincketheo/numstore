#pragma once

#include "common/types.h"
#include "memory/page_alloc.h"

typedef struct {
  page_alloc alloc;
  page_ptr head;
} hash_table;
