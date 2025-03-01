#pragma once

#include "types.h"

/////////////////////// Allocation
void *i_malloc (u64 bytes);
void *i_calloc (u64 n, u64 size);
void i_free (void *ptr);
void *i_realloc (void *ptr, u64 bytes);

// Idea: Program Allocation
// Allocations that you know will be around for the
// entire program - e.g. don't need a free until the end
