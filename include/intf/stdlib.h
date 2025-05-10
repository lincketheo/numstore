#pragma once

#include "intf/types.h"

#include <stdio.h>
#include <string.h> // snprintf

// I just like this pattern just to know
// which functions from the standard library I'm
// using. Not fully ocd about it, it's all iso

#define i_memset(s, c, n) memset (s, c, n)
#define i_memcpy(dest, src, bytes) memcpy (dest, src, bytes)
#define i_memmove(dest, src, bytes) memmove (dest, src, bytes)
#define i_strncmp(left, right, len) strncmp (left, right, len)
#define i_unsafe_strlen(cstr) strlen (cstr)
#define i_memcmp(s1, s2, n) memcmp (s1, s2, n)
#define i_memchr(buf, c, len) memchr (buf, c, len)
#define i_snprintf(buf, len, ...) snprintf (buf, len, __VA_ARGS__)
#define i_vsnprintf(buf, len, ...) vsnprintf (buf, len, __VA_ARGS__)
