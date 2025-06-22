#include "client/repl.h"

#include "client/client.h" // client
#include "dev/assert.h"    // DEFINE_DBG_ASSERT_I
#include "errors/error.h"  // err_t
#include "intf/io.h"
#include "intf/stdlib.h"      // i_memcpy
#include "thirdp/linenoise.h" // linenoise
#include "utils/macros.h"     // arrlen

#include <errno.h>  // strerror
#include <stdlib.h> // free
#include <string.h>

struct repl_s
{
  char *buffer; // A buffer to send over the socket
  u32 blen;     // length of buffer

  const char *ip_addr; // Ip address of server
  const u16 port;      // port of server

  client *client; // On going client object

  bool running;
};

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

static const char *TAG = "Repl";

//////////////////////////////// LINENOISE
static void
completion (const char *buf, linenoiseCompletions *lc)
{
  switch (buf[0])
    {
    case 'c':
      linenoiseAddCompletion (lc, "create");
      break;
    case 'a':
      linenoiseAddCompletion (lc, "append");
      break;
    case 'd':
      linenoiseAddCompletion (lc, "delete");
      break;
    case 'i':
      linenoiseAddCompletion (lc, "insert");
      break;
    case 'r':
      linenoiseAddCompletion (lc, "read");
      break;
    case 't':
      linenoiseAddCompletion (lc, "take");
      break;
    case 'u':
      linenoiseAddCompletion (lc, "update");
      break;
    }
}

static char *
hints (const char *buf, int *color, int *bold)
{
  *color = 35;
  *bold = 0;

  if (strcmp (buf, "c") == 0)
    return "reate";
  if (strcmp (buf, "a") == 0)
    return "ppend";
  if (strcmp (buf, "d") == 0)
    return "elete";
  if (strcmp (buf, "i") == 0)
    return "nsert";
  if (strcmp (buf, "r") == 0)
    return "ead";
  if (strcmp (buf, "t") == 0)
    return "ake";
  if (strcmp (buf, "u") == 0)
    return "pdate";

  return NULL;
}

static inline void
linenoise_config (void)
{
  linenoiseSetCompletionCallback (completion);
  linenoiseSetHintsCallback (hints);
  linenoiseHistoryLoad ("history.txt");
}

//////////////////////////////// REPL

repl *
repl_open (repl_params params, error *e)
{
  repl *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate repl", TAG);
      return NULL;
    }

  repl _ret = {
    .buffer = NULL,
    .blen = 0,
    .ip_addr = params.ip_addr,
    .port = params.port,
    .running = true,
  };

  i_memcpy (ret, &_ret, sizeof _ret);

  ret->client = client_open (params.ip_addr, params.port, e);
  if (ret->client == NULL)
    {
      i_free (ret);
      return NULL;
    }

  linenoise_config ();

  return ret;
}

bool
repl_is_running (repl *r)
{
  repl_assert (r);
  return r->running;
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
  r->running = false;
}

static const reserved rsrvd[] = {
  {
      .cmd = "quit",
      .clen = 4,
      .func = ns_quit,
  },
};

err_t
repl_execute (repl *r, error *e)
{
  repl_assert (r);
  ASSERT (r->running);

  /**
   * Reset buffer
   */
  if (r->buffer)
    {
      free (r->buffer);
      r->buffer = NULL;
      r->blen = 0;
    }

  /**
   * Read input
   */
  errno = 0;
  char *line = linenoise ("numstore> ");
  if (line == NULL)
    {
      if (errno == ENOMEM)
        {
          const char *msg = strerror (errno);
          return error_causef (
              e, ERR_NOMEM,
              "linenoise: %s",
              msg);
        }
      else
        {
          // Considering all other NULL's as done
          r->running = false;
          return SUCCESS;
        }
    }

  r->buffer = line;
  r->blen = strlen (line);
  linenoiseHistoryAdd (line);
  linenoiseHistorySave ("history.txt");

  /**
   * Empty command - do nothing
   */
  if (r->buffer[0] == '\0')
    {
      return SUCCESS;
    }

  /**
   * Execute a reserved command
   */
  if (r->buffer[0] == '.')
    {
      for (u32 i = 0; i < arrlen (rsrvd); ++i)
        {
          reserved rv = rsrvd[i];
          bool equal = r->blen >= rv.clen + 1;
          equal = equal && (strncmp (rv.cmd, &r->buffer[1], rv.clen) == 0);
          if (equal)
            {
              rv.func (r);
              return SUCCESS;
            }
        }
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "Unknown command: %.*s",
          r->blen - 1, &r->buffer[1]);
    }

  /**
   * Send it
   */
  if (r->buffer)
    {
      const string send = (string){ .data = r->buffer, .len = r->blen };
      err_t_wrap (client_send (r->client, send, e), e);
      fprintf (stdout, "Sending: %.*s\n", send.len, send.data);
    }

  /**
   * Recieve it
   */
  string recv = client_recv (r->client, e);
  if (recv.data)
    {
      fprintf (stdout, "Got: %.*s\n", recv.len, recv.data);
      free (recv.data);
    }
  else
    {
      return err_t_from (e);
    }

  return SUCCESS;
}

err_t
repl_close (repl *r, error *e)
{
  repl_assert (r);

  err_t_continue (client_close (r->client, e), e);
  if (r->buffer)
    {
      free (r->buffer);
      r->buffer = NULL;
      r->blen = 0;
    }
}
