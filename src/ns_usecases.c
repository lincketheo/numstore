#include "ns_usecases.h"
#include "ns_data_structures.h"
#include "ns_dtype.h"
#include "ns_errors.h"
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

static ns_ret_t
ns_write_type (FILE *fp, dtype type)
{
  int _type = type;
  buf b = buf_create_from (&_type, sizeof _type, 1, 1);
  return ns_buf_fwrite_out (&b, 1, fp);
}

static ns_ret_t
ns_read_type (FILE *ifp, dtype *type)
{
  ns_ret_t ret;
  int dest;
  buf b = buf_create_from (&dest, sizeof dest, 0, 1);
  if ((ret = ns_buf_fread (&b, 1, ifp)) != NS_OK)
    {
      return ret;
    }
  if (b.len != 1 || inttodtype (type, dest))
    {
      return NS_CORRUPT; // TODO - think about this
    }

  return NS_OK;
}

static ns_FILE_ret
ns_fopen_ds (const char *dbname, const char *dsname, const char *mode)
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

  return ns_fopen (fullname.data, mode);
}

ns_ret_t
ns_create_ds (ns_create_ds_args args)
{
  ns_FILE_ret f = ns_fopen_ds (args.dbname, args.dsname, "w");
  ns_ret_t ret;

  if (f.stat != NS_OK)
    {
      return f.stat;
    }

  if ((ret = ns_write_type (f.fp, args.type) != NS_OK))
    {
      ns_fclose (f.fp); // Swallow exception
      return ret;
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

static ns_ret_t
ns_open_ds (ns_open_ds_ret *dest, const char *dbname, const char *dsname,
            const char *mode)
{
  ns_FILE_ret f = ns_fopen_ds (dbname, dsname, mode);
  ns_ret_t ret;

  if (f.stat != NS_OK)
    {
      return f.stat;
    }

  if ((ret = ns_read_type (f.fp, &dest->h.type)))
    {
      ns_log (ERROR,
              "Failed to read header "
              "from database: %s dataset: %s\n",
              dbname, dsname);
      ns_fclose (f.fp); // Swallow exception
      return ret;
    }

  dest->fp = f.fp;
  return NS_OK;
}

ns_ret_t
ns_ds_fp (ns_ds_fp_args args)
{
  // Validate srange
  srange_ASSERT (args.s);
  ns_ret_t ret;

  ns_open_ds_ret ns_open;
  if ((ret = ns_open_ds (&ns_open, args.dbname, args.dsname, "rb")))
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

ns_ret_t
ns_fp_ds (ns_fp_ds_args args)
{
  // Validate srange
  srange_ASSERT (args.s);
  ns_ret_t ret;

  ns_open_ds_ret ns_open;
  if ((ret = ns_open_ds (&ns_open, args.dbname, args.dsname, "r+")))
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
      size_t read = fread (inbuf, size, 2048, args.fp);
      if (read > 0)
        {
          memcpy (obuf, inbuf, read * size);
          fwrite (obuf, size, read, ns_open.fp);
        }
    }
  while (!feof (args.fp));

  return 0;
}
