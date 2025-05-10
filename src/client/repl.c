#include "client/repl.h"

#include "intf/stdlib.h"   // i_memcpy
#include "utils/macros.h"  // arrlen
#include "utils/strings.h" // line_length

#include <errno.h>  // strerror
#include <stdlib.h> // free

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
repl_create (repl *dest, repl_params params)
{
  ASSERT (dest);

  client c;
  client_create (&c);

  repl ret = {
    .buffer = NULL,
    .blen = 0,
    .ip_addr = params.ip_addr,
    .port = params.port,
    .client = c,
  };

  i_memcpy (dest, &ret, sizeof ret);
}

err_t
repl_read (repl *r, error *e)
{
  repl_assert (r);

  fprintf (stdout, "numstore> ");

  size_t len;

  errno = 0;
  if (getline (&r->buffer, &len, stdin) < 0)
    {
      if (errno == ENOMEM)
        {
          return error_causef (
              e, ERR_NOMEM,
              "getline: %s", strerror (errno));
        }
      else if (errno)
        {
          return error_causef (
              e, ERR_IO,
              "getline: %s", strerror (errno));
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
repl_reserved_execute (repl *r)
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
  return ERR_INVALID_ARGUMENT;
}

err_t
repl_execute (repl *r, error *e)
{
  repl_assert (r);

  err_t ret = SUCCESS;

  // Execute reserved command
  if (r->blen > 0 && r->buffer[0] == '.')
    {
      return repl_reserved_execute (r);
    }

  // Connect
  if ((ret = client_connect (&r->client, r->ip_addr, r->port)))
    {
      return ret;
    }

  // Send
  if (r->buffer)
    {
      i_log_info ("Sending: %.*s\n", r->blen, r->buffer);
      const string send = (string){ .data = r->buffer, .len = r->blen };
      err_t_wrap (client_execute_all (&r->client, send, e), e);
    }

  // Recv
  // Disconnect
  client_disconnect (&r->client);

  free (r->buffer);
  r->buffer = NULL;
  r->blen = 0;

  return SUCCESS;
}
