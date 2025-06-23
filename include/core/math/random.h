#pragma once

#include "core/ds/strings.h" // string
#include "core/intf/types.h" // u32
#include "core/mm/lalloc.h"  // lalloc

void rand_seed (void);
void rand_seed_with (u32 seed);

u32 randu32 (u32 lower, u32 upper);

err_t rand_str (string *dest, lalloc *alloc, u32 minlen, u32 maxlen, error *e);
