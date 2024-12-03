#include "numstore_db.h" // root

#include "common_assert.h"  // ASSERT
#include "common_logging.h" // fprintf_...

#include "numstore_dtype.h"
#include "posix_futils.h" // various file utils

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

//////////////////////////////////////////////////
/////////// CREATE DATABASE

void
nscdb_log (nscdb_args args, nscdb_ret ret)
{
  switch (ret.type)
    {
    case NSCDB_RT_SUCCESS:
      fprintf_dbg ("Create database: %s SUCCESS\n", args.dbname);
      break;
    case NSCDB_RT_FILE_CONFLICT:
      fprintf_err ("Create database: %s FILE CONFLICT\n", args.dbname);
      break;
    case NSCDB_RT_OS_FAILURE:
      fprintf_err ("Create database: %s OS ERROR: %s\n", args.dbname,
                   strerror (ret.errno_from_os_failure));
      break;
    }
}

nscdb_ret
nscdb (nscdb_args args)
{
  ASSERT (args.dbname);
  if (file_exists_any_type (args.dbname))
    {
      return (nscdb_ret){
        .type = NSCDB_RT_FILE_CONFLICT,
      };
    }

  int err;
  if ((err = create_dir (args.dbname, 0755)))
    {
      return (nscdb_ret){
        .type = NSCDB_RT_OS_FAILURE,
        .errno_from_os_failure = err,
      };
    }

  return (nscdb_ret){
    .type = NSCDB_RT_SUCCESS,
  };
}

//////////////////////////////////////////////////
/////////// CREATE DATASET

void
nscds_log (nscds_args args, nscds_ret ret)
{
  switch (ret.type)
    {
    case NSCDS_RT_SUCCESS:
      fprintf_dbg ("Create database: %s SUCCESS\n", args.dsname);
      break;
    case NSCDS_RT_DB_DOESNT_EXIST:
      fprintf_err ("Create dataset: %s db: %s "
                   "doesn't exist\n",
                   args.dsname, args.dbname);
    case NSCDS_RT_CONFLICT:
      fprintf_err ("Create database: %s FILE CONFLICT\n", args.dsname);
      break;
    case NSCDS_RT_OS_FAILURE:
      fprintf_err ("Create dataset: %s "
                   "OS ERROR: %s\n",
                   args.dsname, strerror (ret.errno_from_os_failure));
      break;
    }
}

static inline bool
nsdb_exists (const char *dbname)
{
  return dir_exists (dbname);
}

static inline bool
nsds_exists (const char *dbname, const char *dsname)
{
  return file_exists_in_dir (dbname, dsname);
}

nscds_ret
nscds (nscds_args args)
{
  ASSERT (args.dbname);
  ASSERT (args.dsname);
  ASSERT (is_dtype_valid (args.type));

  // Check if db exists
  if (!nsdb_exists (args.dbname))
    {
      return (nscds_ret){
        .type = NSCDS_RT_DB_DOESNT_EXIST,
      };
    }

  // Check if ds exists
  if (nsds_exists (args.dbname, args.dsname))
    {
      return (nscds_ret){
        .type = NSCDS_RT_CONFLICT,
      };
    }

  // Create filename
  char fullname[2048];
  if (path_join (fullname, 2048, args.dbname, args.dsname))
    {
      return (nscds_ret){
        .type = NSCDS_RT_NAMES_TOO_LONG,
      };
    }
}
