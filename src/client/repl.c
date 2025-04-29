#include "client/repl.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "utils/bounds.h"
#include "utils/macros.h"
#include "utils/strings.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_DBG_ASSERT_I (repl, repl, r)
{
  ASSERT (r);
  if (r->blen == 0)
    {
      ASSERT (r->buffer == NULL);
    }
  else
    {
      ASSERT (r->buffer);
    }
}

void
repl_create (repl *dest)
{
  ASSERT (dest);
  dest->buffer = NULL;
  dest->blen = 0;
}

err_t
repl_read (repl *r)
{
  repl_assert (r);

  fprintf (stdout, "numstore> ");

  size_t len;

  errno = 0;
  if (getline (&r->buffer, &len, stdin) < 0)
    {
      if (errno == ENOMEM)
        {
          return ERR_NOMEM;
        }
      else if (errno)
        {
          perror ("getline");
          return ERR_IO;
        }
      // EOF
      return SUCCESS;
    }

  r->blen = line_length (r->buffer, len);

  repl_assert (r);

  return SUCCESS;
}

typedef struct
{
  const char *cmd;
  u32 clen;
  void (*func) (repl *);
} reserved;

void
ns_quit (repl *r)
{
  repl_assert (r);
  if (r->buffer)
    {
      free (r->buffer);
    }
  r->buffer = NULL;
  r->blen = 0;
  exit (0);
}

static const reserved rsrvd[] = {
  { .cmd = "quit",
    .clen = 4,
    .func = ns_quit },
};

err_t
repl_execute (repl *r)
{
  repl_assert (r);

  if (r->blen > 0 && r->buffer[0] == '.')
    {
      for (u32 i = 0; i < arrlen (rsrvd); ++i)
        {
          reserved rv = rsrvd[i];
          bool equal = rv.clen == r->blen - 1;
          equal = equal && (strncmp (rv.cmd, &r->buffer[1], rv.clen) == 0);
          if (equal)
            {
              rv.func (r);
              return SUCCESS;
            }
        }
    }

  if (r->buffer)
    {
      fprintf (stdout, "%.*s\n", r->blen, r->buffer);
    }

  free (r->buffer);
  r->buffer = NULL;
  r->blen = 0;

  return SUCCESS;
}
