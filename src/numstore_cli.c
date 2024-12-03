#include "numstore_cli.h" // Root

#include "numstore_db.h"
#include "numstore_dtype.h" // dtype

#include "common_assert.h"  // ASSERT
#include "common_logging.h" // fprintf_err
#include "common_string.h"  // safe_strequal

//////////////////////////////////////////////////
/////////// cli_state (A simple utility to help pop cli states)
/// static inline because they're short enough

typedef struct
{
  int argc;
  char **args;
  const char *prgrm;
} cli_state;

typedef struct
{
  cli_state next;
  const char *head;
} cli_state_pop;

static inline int
cli_state_more (cli_state state)
{
  if (state.argc == 0)
    {
      ASSERT (state.args == NULL);
      return 0;
    }

  ASSERT (state.args != NULL);
  return 1;
}

static inline cli_state
cli_state_init (int argc, char **argv)
{
  ASSERT (argc >= 1);
  ASSERT (argv != NULL);

  char **nargs = argc == 1 ? NULL : &argv[1];
  return (cli_state){
    .argc = argc - 1,
    .args = nargs,
    .prgrm = argv[0],
  };
}

static inline cli_state_pop
cli_state_pop_next (cli_state prev)
{
  if (!cli_state_more (prev))
    {
      ASSERT (prev.args == NULL);
      return (cli_state_pop){
        .head = NULL,
        .next = prev,
      };
    }

  char **nargs = prev.argc == 1 ? NULL : &prev.args[1];

  return (cli_state_pop) {
    .head = prev.args[0],
    .next = (cli_state){
      .args = nargs,
      .argc = prev.argc - 1,
      .prgrm = prev.prgrm,
    },
  };
}

//////////////////////////////////////////////////
/////////// Root Actions (entry into usecases)
/// static inline because they're only called in one
/// call graph

static inline int
handle_create_db (const char *dbname)
{
  fprintf_err ("Creating database %s\n", dbname);
  return 0;
}

static inline int
handle_create_ds (const char *dbname, const char *dsname, dtype type)
{
  const nscds_args args = {
    .type = type,
    .dsname = dsname,
    .dbname = dbname,
  };
  const nscds_ret ret = nscds (args);
  nscds_log (args, ret);
  return ret.type == NSCDS_RT_SUCCESS;
}

static inline int
handle_delete_ds (const char *dbname, const char *dsname)
{
  fprintf_err ("Reading from dataset: %s\n", dsname);
  return 0;
}

static inline int
handle_read_ds (const char *dbname, const char *dsname, const char *rng_str)
{
  fprintf_err ("Reading from dataset: %s\n", dsname);
  return 0;
}

static inline int
handle_write_ds (const char *dbname, const char *dsname, const char *rng_str)
{
  fprintf_err ("Writing to dataset: %s\n", dsname);
  return 0;
}

static inline int
handle_append_ds (const char *dbname, const char *dsname, const char *rng_str)
{
  fprintf_err ("Appending to dataset: %s\n", dsname);
  return 0;
}

//////////////////////////////////////////////////
/////////// Handler functions
/// static inline because they're only called in one
/// call graph

static inline int
handle_db_create_args (cli_state state)
{
  cli_state_pop next = (cli_state_pop){ .next = state };
  const char *dbname = (next = cli_state_pop_next (next.next)).head;

  if (!dbname)
    goto failed;

  return handle_create_db (dbname);

failed:
  fprintf_err ("Usage: %s db create DBNAME\n", state.prgrm);
  return -1;
}

static inline int
handle_ds_create_args (cli_state state)
{
  cli_state_pop next = (cli_state_pop){ .next = state };
  const char *dbname = (next = cli_state_pop_next (next.next)).head;
  const char *dsname = (next = cli_state_pop_next (next.next)).head;
  const char *typestr = (next = cli_state_pop_next (next.next)).head;

  if (!dbname || !dsname || !typestr)
    goto failed;

  dtype_result dtr = strtodtype (typestr);
  if (!dtr.ok)
    {
      fprintf_err ("Invalid dtype: %s\n", typestr);
      goto failed;
    }

  return handle_create_ds (dbname, dsname, dtr.v);

failed:
  fprintf_err ("Usage: %s ds create DBNAME DSNAME DTYPE\n", state.prgrm);
  return -1;
}

static inline int
handle_ds_delete_args (cli_state state)
{
  cli_state_pop next = (cli_state_pop){ .next = state };
  const char *dbname = (next = cli_state_pop_next (next.next)).head;
  const char *dsname = (next = cli_state_pop_next (next.next)).head;

  if (!dbname || !dsname)
    goto failed;

  return handle_delete_ds (dbname, dsname);

failed:
  fprintf_err ("Usage: %s ds delete DBNAME DSNAME\n", state.prgrm);
  return -1;
}

static inline int
handle_ds_append_args (cli_state state)
{
  cli_state_pop next = (cli_state_pop){ .next = state };
  const char *dbname = (next = cli_state_pop_next (next.next)).head;
  const char *dsname = (next = cli_state_pop_next (next.next)).head;
  const char *rng_str = (next = cli_state_pop_next (next.next)).head;

  if (!dbname || !dsname)
    goto failed;

  return handle_append_ds (dbname, dsname, rng_str);

failed:
  fprintf_err ("Usage: %s ds read DBNAME DSNAME [OPTIONS...]\n", state.prgrm);
  return -1;
}

static inline int
handle_ds_write_args (cli_state state)
{
  cli_state_pop next = (cli_state_pop){ .next = state };
  const char *dbname = (next = cli_state_pop_next (next.next)).head;
  const char *dsname = (next = cli_state_pop_next (next.next)).head;
  const char *rng_str = (next = cli_state_pop_next (next.next)).head;

  if (!dbname || !dsname)
    goto failed;

  return handle_write_ds (dbname, dsname, rng_str);

failed:
  fprintf_err ("Usage: %s ds read DBNAME DSNAME [OPTIONS...]\n", state.prgrm);
  return -1;
}

static inline int
handle_ds_read_args (cli_state state)
{
  cli_state_pop next = (cli_state_pop){ .next = state };
  const char *dbname = (next = cli_state_pop_next (next.next)).head;
  const char *dsname = (next = cli_state_pop_next (next.next)).head;
  const char *rng_str = (next = cli_state_pop_next (next.next)).head;

  if (!dbname || !dsname)
    goto failed;

  return handle_read_ds (dbname, dsname, rng_str);

failed:
  fprintf (stderr,
           "Usage: %s ds read DBNAME DSNAME "
           "[RANGE STRING]\n",
           state.prgrm);
  return -1;
}

static inline int
handle_db_args (cli_state state)
{
  if (!cli_state_more (state))
    {
      goto failed;
    }

  cli_state_pop next = cli_state_pop_next (state);

  if (safe_strequal (next.head, "create"))
    {
      return handle_db_create_args (next.next);
    }

failed:
  fprintf (stderr,
           "Usage: %s db create "
           "[OPTIONS...]\n",
           state.prgrm);
  return -1;
}

static inline int
handle_ds_args (cli_state state)
{
  if (!cli_state_more (state))
    {
      goto failed;
    }

  cli_state_pop next = cli_state_pop_next (state);

  if (safe_strequal (next.head, "create"))
    {
      return handle_ds_create_args (next.next);
    }
  else if (safe_strequal (next.head, "delete"))
    {
      return handle_ds_delete_args (next.next);
    }
  else if (safe_strequal (next.head, "read"))
    {
      return handle_ds_read_args (next.next);
    }
  else if (safe_strequal (next.head, "write"))
    {
      return handle_ds_write_args (next.next);
    }
  else if (safe_strequal (next.head, "append"))
    {
      return handle_ds_append_args (next.next);
    }

failed:
  fprintf (stderr,
           "Usage: %s ds [create|delete|read|write|append] "
           "[OPTIONS...]\n",
           state.prgrm);
  return -1;
}

int
handle_args (int argc, char **argv)
{
  cli_state state = cli_state_init (argc, argv);
  if (!cli_state_more (state))
    {
      goto failed;
    }

  cli_state_pop next = cli_state_pop_next (state);

  if (safe_strequal (next.head, "db"))
    {
      return handle_db_args (next.next);
    }
  else if (safe_strequal (next.head, "ds"))
    {
      return handle_ds_args (next.next);
    }

failed:
  fprintf_err ("Usage: %s [db|ds] [OPTIONS...]\n", state.prgrm);
  return -1;
}
