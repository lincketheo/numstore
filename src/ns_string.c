#include "ns_string.h"
#include "ns_memory.h"

#include <string.h> // strnlen

string
string_from_cstr (char *str)
{
  ns_size len = strnlen (str, MAX_STRLEN);
  return (string){
    .data = str,
    .len = len,
    .iscstr = len != MAX_STRLEN,
  };
}

ns_size
path_join_len (string prefix, string suffix)
{
  return prefix.len + suffix.len + 10;
}

ns_ret_t
path_join (string *dest, ns_alloc *a, const string prefix, const string suffix)
{
  ASSERT (a);
  ASSERT (prefix.data);
  ASSERT (suffix.data);

  // bytes handles for memory ops
  bytes _prefix = stringtobytes (prefix);
  bytes _suffix = stringtobytes (suffix);
  bytes combined = bytes_create_empty ();

  // Allocate memory for return buffer
  ns_size cap = path_join_len (prefix, suffix);
  ns_ret_t ret = a->ns_malloc (a, &combined, cap);

  if (ret != NS_OK)
    {
      return ret;
    }

  // Insert first string
  ns_memcpy (&combined, &_prefix, prefix.len * sizeof *prefix.data);

  // Add path seperator
  ns_size len = prefix.len;
  if (dest->data[len] != PATH_SEPARATOR)
    {
      dest->data[len++] = PATH_SEPARATOR;
    }

  // Insert second string
  bytes next = bytes_from (combined, len);
  ns_memcpy (&next, &_suffix, suffix.len);

  // Add null terminator for reliability
  len += suffix.len;
  dest->data[len] = '\0';
  dest->iscstr = true;

  return NS_OK;
}

bytes
stringtobytes (string s)
{
  return bytes_create_from ((ns_byte *)s.data, s.len * sizeof *s.data);
}
