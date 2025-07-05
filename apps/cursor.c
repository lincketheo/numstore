
#include "numstore/cursor/cursor.h"
#include "compiler/ast/query.h"
#include "core/dev/assert.h"
#include "core/ds/cbuffer.h"
#include "core/ds/strings.h"
#include "core/errors/error.h"
#include "core/intf/io.h"
#include "core/utils/macros.h"
#include "numstore/paging/pager.h"
#include "numstore/type/types.h"

#include <stdio.h>
#include <stdlib.h>

#define err_t_abort(expr, e)                         \
  do                                                 \
    {                                                \
      err_t __ret = (err_t)expr;                     \
      if (__ret < SUCCESS)                           \
        {                                            \
          i_log_error ("Failing from: %s\n", #expr); \
          exit (-1);                                 \
        }                                            \
    }                                                \
  while (0)

#define err_t_null_check(expr)                                  \
  do                                                            \
    {                                                           \
      if (expr == NULL)                                         \
        {                                                       \
          i_log_error ("Failing from null value: %s\n", #expr); \
          exit (-1);                                            \
        }                                                       \
    }                                                           \
  while (0)

error _e;
error *e = &_e;
cursor *c;
pager *p;
u32 write_data[2048];
u32 insert_data[] = { 20, 90, 11, 10, 1 };
u32 read_data[2048];

void
_cursor_open ()
{
  err_t_abort (i_remove_quiet (unsafe_cstrfrom ("test.db"), e), e);
  p = pgr_open (unsafe_cstrfrom ("test.db"), e);
  err_t_null_check (p);

  c = cursor_open (p, e);
  err_t_null_check (c);
}

void
_cursor_close ()
{
  cursor_close (c);
  pgr_close (p, e);
}

void
init_write_data ()
{
  for (u32 i = 0; i < arrlen (write_data); ++i)
    {
      write_data[i] = i;
    }
}

int
main ()
{
  _e = error_create (NULL);

  _cursor_open ();

  create_query cq = {
    .vname = unsafe_cstrfrom ("foobar"),
    .type = (type){
        .type = T_PRIM,
        .p = U32,
    },
  };

  init_write_data ();
  cbuffer source = cbuffer_create_full_from (write_data);
  cbuffer dest = cbuffer_create_from (read_data);
  cbuffer insert_source = cbuffer_create_full_from (insert_data);

  err_t_abort (cursor_create (c, cq.vname, cq.type, e), e);
  err_t_abort (cursor_insert (c, cq.vname, 0, arrlen (write_data), &source, e), e);

  while (!cursor_idle (c))
    {
      err_t_abort (cursor_execute (c, e), e);
    }

  err_t_abort (cursor_write (c, cq.vname, 5, arrlen (insert_data), 3, &insert_source, e), e);

  while (!cursor_idle (c))
    {
      err_t_abort (cursor_execute (c, e), e);
    }

  err_t_abort (cursor_read (c, cq.vname, 0, arrlen (read_data), 1, &dest, e), e);

  while (!cursor_idle (c))
    {
      err_t_abort (cursor_execute (c, e), e);
    }

  for (u32 i = 0; i < arrlen (read_data) - 1; ++i)
    {
      printf ("%d\n", read_data[i]);
    }
  printf ("\n");

  _cursor_close ();
}
