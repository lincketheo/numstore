#pragma once

#include "types.h"

void *i_memset (void *s, int c, u64 n);
void *i_memcpy (void *dest, const void *src, u64 bytes);
int i_unsafe_strlen (const char *cstr);
int i_memcmp (const void *s1, const void *s2, u64 n);
