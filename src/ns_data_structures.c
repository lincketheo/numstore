#include "ns_data_structures.h"
#include "ns_errors.h"
#include "ns_memory.h"

#include <string.h>

string
string_from_cstr (const char *str)
{
  size_t len = strnlen (str, MAX_STRLEN);
  return (string){
    .data = str,
    .len = len,
    .iscstr = len != MAX_STRLEN,
  };
}

size_t
path_join_len (string prefix, string suffix)
{
  return prefix.len + suffix.len + 10;
}

string
path_join (ns_alloc *a, string prefix, string suffix)
{
  ASSERT (a);
  ASSERT (prefix.data);
  ASSERT (suffix.data);

  char *dest = ns_alloc_malloc (a, path_join_len (prefix, suffix));
  memcpy (dest, prefix.data, prefix.len);

  size_t len = prefix.len;
  if (dest[len] != PATH_SEPARATOR)
    {
      dest[len++] = PATH_SEPARATOR;
    }

  memcpy (&dest[len], suffix.data, suffix.len);
  len += suffix.len;
  dest[len] = '\0';
  return (string){
    .data = dest,
    .len = len + len,
    .iscstr = 1,
  };
}
