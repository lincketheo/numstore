#include "ast/query/qspace_provider.h"
#include "compiler/compiler.h"
#include "server/connection.h"

#include "compiler/parser.h"  // parser
#include "compiler/scanner.h" // scanner
#include "sckctrl.h"          // sckctrl

#include "ds/cbuffer.h"   // cbuffer
#include "errors/error.h" // err_t
#include "intf/io.h"      // i_file

#include <stdlib.h>   // malloc / free
#include <sys/poll.h> // pollfd

DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
}

struct connection_s
{
  bool want_close;

  /**
   * Workers
   */
  sckctrl socket;
  compiler compiler;

  /**
   * Shared data buffers
   */
  cbuffer input;   // Recieve Buffer
  cbuffer queries; // Output from parser

  u8 _input[20];
  query *_queries[10];

  qspce_prvdr *qspcp;
};

connection *
con_create (i_file cfd, struct sockaddr_in caddr)
{
  connection *ret = malloc (sizeof *ret);

  if (ret == NULL)
    {
      return NULL;
    }

  qspce_prvdr *qspcp = qspce_prvdr_create ();
  if (qspcp == NULL)
    {
      free (ret);
      return NULL;
    }

  ret->want_close = false;

  ret->socket = sckctrl_create (cfd, caddr, &ret->input, NULL);

  ret->input = cbuffer_create_from (ret->_input);
  ret->queries = cbuffer_create_from (ret->_queries);

  ret->qspcp = qspcp;

  compiler_create (&ret->compiler, &ret->input, &ret->queries, qspcp);

  connection_assert (ret);

  return ret;
}

bool
con_is_done (connection *c)
{
  return c->want_close;
}

void
con_free (connection *c)
{
  connection_assert (c);
}

void
con_disconnect (connection *c)
{
  connection_assert (c);
  sckctrl_close (&c->socket);
  free (c);
}

struct pollfd
con_to_pollfd (const connection *src)
{
  connection_assert (src);

  struct pollfd ret = { src->socket.cfd.fd, POLLERR, 0 };
  ret.events |= POLLIN;
  return ret;
}

void
con_read (connection *c)
{
  connection_assert (c);

  error e = error_create (NULL);
  if (sckctrl_read (&c->socket, &e))
    {
      error_log_consume (&e);
      c->want_close = true;
    }
}

void
con_execute (connection *c)
{
  connection_assert (c);
  compiler_execute (&c->compiler, NULL);
}

void
con_write (connection *c)
{
  connection_assert (c);
  panic ();
}
