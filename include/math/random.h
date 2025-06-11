#pragma once

#include "ds/strings.h" // string
#include "intf/types.h" // u32
#include "mm/lalloc.h"  // lalloc

u32 randu32 (u32 lower, u32 upper);

err_t rand_str (string *dest, lalloc *alloc, u32 minlen, u32 maxlen, error *e);
