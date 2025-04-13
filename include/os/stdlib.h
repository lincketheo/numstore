#pragma once

#include "common/types.h"
#include <string.h>

void *i_memset (void *s, int c, u64 n);

void *i_memcpy (void *dest, const void *src, u64 bytes);
