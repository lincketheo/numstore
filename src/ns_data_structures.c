#include "ns_data_structures.h"
#include "ns_errors.h"
#include "ns_memory.h"

#include <string.h>

size_t
buf_avail (buf b)
{
  buf_ASSERT (&b);
  return b.cap - b.len;
}

void
buf_shift_mem (buf *b, size_t ind)
{
  buf_ASSERT (b);
  ASSERT (ind <= b->len);
  if (ind == b->len)
    {
      b->len = 0;
      return;
    }

  size_t tomove = b->len - ind;
  memmove (b->data, b->data + ind * b->size, tomove * b->size);
  b->len -= tomove;
}

static char qbdata[2048];

buf
quick_buf (size_t size, size_t cap)
{
  ASSERT (cap * size < sizeof (qbdata));
  return (buf){
    .data = qbdata,
    .len = 0,
    .cap = cap,
    .size = size,
  };
}

buf
buf_create_from (void *data, size_t size, size_t len, size_t cap)
{
  return (buf){
    .len = len,
    .cap = cap,
    .size = size,
    .data = data,
  };
}

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
