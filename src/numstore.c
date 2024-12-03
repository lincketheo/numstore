#include "numstore.h"

#include <bits/types/FILE.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

char quick_heap[2048];

///////////////////////////////
////////// Memory
size_t
quick_heap_start ()
{
  return 0;
}

int
quick_heap_avail (size_t ctx, size_t bytes)
{
  return (ctx + bytes) < sizeof (quick_heap);
}

void *
quick_heap_malloc (size_t *ctx, size_t bytes)
{
  ASSERT (quick_heap_avail (*ctx, bytes) && "Quick Heap Overflow");
  void *ret = &quick_heap[*ctx];
  *ctx += bytes;
  return ret;
}

typedef struct
{
  void *data;
  const size_t cap;
  size_t len;
} linalloc;

linalloc
linalloc_create_qh ()
{
  return (linalloc){
    .data = quick_heap,
    .cap = sizeof (quick_heap),
    .len = 0,
  };
}

int
linalloc_avail (linalloc *l, size_t bytes)
{
  ASSERT (l);
  return l->len + bytes <= l->cap;
}

void *
linalloc_malloc (linalloc *l, size_t bytes)
{
  ASSERT (linalloc_avail (l, bytes));
  void *ret = &((char *)l->data)[l->len];
  l->len += bytes;
  return ret;
}

static inline void
linalloc_free (linalloc *l, void *ptr)
{
  return;
}

typedef enum
{
  LINEAR,
} ns_alloc_type;

typedef struct
{
  union
  {
    linalloc l;
  };
  ns_alloc_type type;
} ns_alloc;

int
ns_alloc_avail (ns_alloc *a, size_t bytes)
{
  ASSERT (a);

  switch (a->type)
    {
    case LINEAR:
      return linalloc_avail (&a->l, bytes);
    default:
      unreachable ();
    }
}

ns_alloc
ns_alloc_create_qh (ns_alloc_type type)
{
  switch (type)
    {
    case LINEAR:
      return (ns_alloc){
        .l = linalloc_create_qh (),
        .type = type,
      };
    default:
      unreachable ();
    }
}

void *
ns_alloc_malloc (ns_alloc *a, size_t bytes)
{
  ASSERT (a);

  switch (a->type)
    {
    case LINEAR:
      return linalloc_malloc (&a->l, bytes);
    default:
      unreachable ();
    }
}

void
ns_alloc_free (ns_alloc *a, void *ptr)
{
  ASSERT (a);
  ASSERT (ptr);

  switch (a->type)
    {
    case LINEAR:
      linalloc_free (&a->l, ptr);
      break;
    default:
      unreachable ();
    }
}

///////////////////////////////
////////// dtype

int
inttodtype (dtype *dest, int type)
{
  ASSERT (dest);
  if (type < 0)
    return -1;
  if (type > 16)
    return -1;
  *dest = type;
  return 0;
}

#define S8                                                                    \
  U8:                                                                         \
  case I8

#define S16                                                                   \
  U16:                                                                        \
  case I16:                                                                   \
  case F16:                                                                   \
  case CI16:                                                                  \
  case CU16

#define S32                                                                   \
  U32:                                                                        \
  case I32:                                                                   \
  case CF32:                                                                  \
  case CI32:                                                                  \
  case CU32:                                                                  \
  case F32

#define S64                                                                   \
  U64:                                                                        \
  case I64:                                                                   \
  case CF64:                                                                  \
  case CI64:                                                                  \
  case CU64:                                                                  \
  case F64

#define S128                                                                  \
  CF128:                                                                      \
  case CI128:                                                                 \
  case CU128

size_t
dtype_sizeof (dtype d)
{
  switch (d)
    {
    case S8:
      return 8;
    case S16:
      return 16;
    case S32:
      return 32;
    case S64:
      return 64;
    case S128:
      return 128;
    default:
      unreachable ();
    }
}

///////////////////////////////
////////// String Ops

#define MAX_STRLEN 1000
#define PATH_SEPARATOR '/'

typedef struct
{
  const char *data;
  size_t len;
  int iscstr;
} string;

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

///////////////////////////////
////////// Database

int
ns_create_db (ns_create_db_args args)
{
  errno = 0;
  if (mkdir (args.dbname, 0755))
    {
      ns_error_report (ns_error_errno (errno));
      return -1;
    }
  return 0;
}

static FILE *
ns_fopen_ds (const char *dbname, const char *dsname)
{
  ns_alloc a = ns_alloc_create_qh (LINEAR);

  const string prefix = string_from_cstr (dbname);
  const string suffix = string_from_cstr (dsname);

  if (!ns_alloc_avail (&a, path_join_len (prefix, suffix)))
    {
      TODO ("Error handle");
      return NULL;
    }

  string fullname = path_join (&a, prefix, suffix);
  ASSERT (fullname.iscstr);
  FILE *fp = fopen (fullname.data, "wb");

  return fp;
}

int
ns_create_ds (ns_create_ds_args args)
{
  FILE *fp = ns_fopen_ds (args.dbname, args.dsname);
  if (fp == NULL)
    {
      TODO ("Error handle");
      return -1;
    }
  if (fclose (fp))
    {
      TODO ("Error handle");
      return -1;
    }
  return 0;
}

typedef struct
{
  dtype type;
} ds_header;

typedef struct
{
  FILE *fp;
  ds_header h;
} ns_open_ds_ret;

static inline int
ns_read_header_from_fp (ds_header *dest, FILE *fp)
{
  ASSERT (dest);
  ASSERT (fp);

  int type;
  size_t read = fread (&dest->type, sizeof dest->type, 1, fp);

  if (read != 1)
    {
      TODO ("Error handle");
      return -1;
    }

  if (inttodtype (&dest->type, type))
    {
      TODO ("Error handle");
      return -1;
    }

  return 0;
}

static int
ns_open_ds (ns_open_ds_ret *dest, const char *dbname, const char *dsname)
{
  FILE *fp = ns_fopen_ds (dbname, dsname);
  if (fp == NULL)
    {
      TODO ("Error handle");
      return -1;
    }

  if (ns_read_header_from_fp (&dest->h, fp))
    {
      TODO ("Error handle");
      return -1;
    }

  return 0;
}

int
ns_read_srange_ds_fp (ns_read_srange_ds_fp_args args)
{
  // Validate srange
  srange_ASSERT (args.s);

  ns_open_ds_ret ns_open;
  if (ns_open_ds (&ns_open, args.dbname, args.dsname))
    {
      TODO ("Error handle");
      return -1;
    }

  size_t size = dtype_sizeof (ns_open.h.type);
  char inbuf[2048];
  char obuf[2048];

  do
    {
      size_t read = fread (inbuf, size, 2048, ns_open.fp);
      if (read > 0)
        {
          memcpy (obuf, inbuf, read * size);
          fwrite (obuf, size, read, args.fp);
        }
    }
  while (!feof (ns_open.fp));

  return 0;
}

///////////////////////////////
////////// Error handling

void
ns_error_report (ns_error e)
{
  fprintf (stderr, "%s\n", strerror (e._errno));
}

ns_error
ns_error_errno (int _errno)
{
  return (ns_error){
    ._errno = _errno,
  };
}

///////////////////////////////
////////// srange

void
nsdb_ds_init (FILE *fp, dtype type)
{
  int _type = type;
  fwrite (&_type, sizeof _type, 1, fp);
}

int
dtype_fread (dtype *dest, FILE *fp)
{
  ASSERT (dest);
  ASSERT (fp);
  int _type;
  size_t read = fread (&_type, sizeof _type, 1, fp);
  if (read != 1)
    {
      return -1;
    }
  return inttodtype (dest, _type);
}
