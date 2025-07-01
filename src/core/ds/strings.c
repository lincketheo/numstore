#include "core/ds/strings.h" // string

#include "core/dev/assert.h"  // DEFINE_DBG_ASSERT_I
#include "core/dev/testing.h" // TEST
#include "core/intf/stdlib.h" // i_memcmp

DEFINE_DBG_ASSERT_I (string, string, s)
{
  ASSERT (s);
  ASSERT (s->data);
  ASSERT (s->len > 0);
}

DEFINE_DBG_ASSERT_I (string, cstring, s)
{
  string_assert (s);
  ASSERT (s->data[s->len] == 0);
}

string
unsafe_cstrfrom (char *cstr)
{
  return (string){
    .data = cstr,
    .len = i_unsafe_strlen (cstr)
  };
}
