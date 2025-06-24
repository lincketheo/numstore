#pragma once

#include <stdio.h>  // TODO
#include <string.h> // snprintf

#include "core/intf/types.h" // TODO

#define i_memset(s, c, n) memset (s, c, n)
#define i_memcpy(dest, src, bytes) memcpy (dest, src, bytes)
#define i_memmove(dest, src, bytes) memmove (dest, src, bytes)
#define i_strncmp(left, right, len) strncmp (left, right, len)
#define i_unsafe_strlen(cstr) strlen (cstr)
#define i_memcmp(s1, s2, n) memcmp (s1, s2, n)
#define i_memchr(buf, c, len) memchr (buf, c, len)
#define i_snprintf(buf, len, ...) snprintf (buf, len, __VA_ARGS__)
#define i_vsnprintf(buf, len, ...) vsnprintf (buf, len, __VA_ARGS__)
