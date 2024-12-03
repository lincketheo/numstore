#pragma once

#include "common_assert.h"

#include <string.h> // various stdlib string operations

#define DEFAULT_MAX_STREQUAL 10

static inline int
safe_strnequal (const char *a, const char *b, size_t max_len)
{
  ASSERT (strnlen (a, max_len) < max_len && "max_len overflow");
  ASSERT (strnlen (b, max_len) < max_len && "max_len overflow");

  return strncmp (a, b, max_len) == 0;
}

#define safe_strequal(a, b) safe_strnequal (a, b, DEFAULT_MAX_STREQUAL)

int path_join (char *dest, size_t dlen, const char *prefix,
               const char *suffix);
