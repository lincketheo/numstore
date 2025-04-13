#pragma once

#include "os/types.h"

/////////////////////// Allocation
void *i_malloc (u64 bytes);

void *i_calloc (u64 n, u64 size);

void i_free (void *ptr);

void *i_realloc (void *ptr, int bytes);
