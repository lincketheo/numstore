#pragma once

#include "intf/types.h"

// For now - not wrapping these two
#include <math.h>
#include <stdlib.h>

void *i_memset (void *s, int c, u64 n);
void *i_memcpy (void *dest, const void *src, u64 bytes);
int i_strncmp (char *left, char *right, u64 len);
int i_unsafe_strlen (const char *cstr);
int i_memcmp (const void *s1, const void *s2, u64 n);
