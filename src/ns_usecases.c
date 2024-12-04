#include "ns_usecases.h"
#include "ns_dtype.h"
#include "ns_memory.h"
#include "posixf.h"
#include "stdlibf.h"

#include <errno.h>
#include <string.h>

///////////////////////////////
////////// Database

ns_ret_t
ns_create_db (ns_create_db_args args)
{
  return ns_mkdir (args.dbname, 0755);
}

static ns_FILE_ret
ns_fopen_ds (const char *dbname, const char *dsname)
{
  ns_alloc a = ns_alloc_create_qh (LINEAR);

  const string prefix = string_from_cstr (dbname);
  const string suffix = string_from_cstr (dsname);

  if (!ns_alloc_avail (&a, path_join_len (prefix, suffix)))
    {
      ns_log (ERROR,
              "Database: %s and "
              "Dataset: %s are too long\n",
              dbname, dsname);
      return (ns_FILE_ret){
        .stat = NS_EINVAL,
      };
    }

  string fullname = path_join (&a, prefix, suffix);
  ASSERT (fullname.iscstr);

  return ns_fopen (fullname.data, "wb");
}

ns_ret_t
ns_create_ds (ns_create_ds_args args)
{
  ns_FILE_ret f = ns_fopen_ds (args.dbname, args.dsname);

  if (f.stat != NS_OK)
    {
      return f.stat;
    }

  return ns_fclose (f.fp);
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

static inline ns_ret_t
ns_read_header_from_fp (ds_header *dest, FILE *fp)
{
  ASSERT (dest);
  ASSERT (fp);

  int type;
  size_t read = fread (&dest->type, sizeof dest->type, 1, fp);

  if (read != 1)
    {
      return -1;
    }

  if (inttodtype (&dest->type, type))
    {
      TODO ("Error handle");
      return -1;
    }

  return 0;
}

static ns_ret_t
ns_open_ds (ns_open_ds_ret *dest, const char *dbname, const char *dsname)
{
  ns_FILE_ret f = ns_fopen_ds (dbname, dsname);
  if (f.stat != NS_OK)
    {
      return f.stat;
    }

  if (ns_read_header_from_fp (&dest->h, f.fp))
    {
      TODO ("Error handle");
      return -1;
    }

  return 0;
}

ns_ret_t
ns_read_srange_ds_fp (ns_read_srange_ds_fp_args args)
{
  // Validate srange
  srange_ASSERT (args.s);
  ns_ret_t ret;

  ns_open_ds_ret ns_open;
  if ((ret = ns_open_ds (&ns_open, args.dbname, args.dsname)))
    {
      ns_log (ERROR,
              "Failed to open database: %s "
              "dataset: %s\n",
              args.dbname, args.dsname);
      return ret;
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
