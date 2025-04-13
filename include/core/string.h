#pragma once

#include "dev/assert.h"
#include "os/stdlib.h"

/////// STRINGS
typedef struct
{
  char *data;
  int len;
} string;

#define cstrfrom(cstr)             \
  (string)                         \
  {                                \
    .data = cstr,                  \
    .len = i_unsafe_strlen (cstr), \
  }

static inline DEFINE_DBG_ASSERT_I (string, string, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->len > 0);
}

static inline DEFINE_DBG_ASSERT_I (string, cstring, s)
{
  string_assert (s);
  ASSERT (s->data[s->len] == 0);
}
