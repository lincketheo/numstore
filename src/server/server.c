#include "server/server.h"
#include "errors/error.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "server/connector.h"
#include "utils/bounds.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

static inline err_t
server_create_connectors (
    server *s,
    server_params p,
    error *e)
{
  lalloc_r cons = lcalloc (p.alloc, 10, 10, sizeof *s->cons);
  if (cons.stat != AR_SUCCESS)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Not enough memory in the server "
          "for connections buffer");
    }

  s->cons = cons.ret;
  s->ccap = cons.rlen;
  s->alloc = p.alloc;

  err_t ret;
  u32 i = 0;

  for (; i < s->ccap; ++i)
    {
      con_params cparams = {
        .alloc = s->alloc,
      };
      if ((ret = con_create (&s->cons[i], cparams, e)))
        {
          goto failed;
        }
    }

failed:
  for (u32 j = 0; j < i; ++j)
    {
      con_free (&s->cons[j]);
    }
  lfree (s->alloc, s->cons);
  return ret;
}

static inline void
server_free_connectors (server *s)
{
  for (u32 j = 0; j < s->ccap; ++j)
    {
      ASSERT (!con_is_open (&s->cons[j]));
      con_free (&s->cons[j]);
    }
  lfree (s->alloc, s->cons);
}

static inline err_t
server_connect (server *s, server_params params, error *e)
{
  int fd;
  struct sockaddr_in addr;
  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      return error_causef (e, ERR_IO, "socket: %s", strerror (errno));
    }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (params.port);

  if (bind (fd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      close (fd);
      return error_causef (e, ERR_IO, "bind: %s", strerror (errno));
    }

  if (listen (fd, 3) < 0)
    {
      close (fd);
      return error_causef (e, ERR_IO, "listen: %s", strerror (errno));
    }

  s->fd = (i_file){ .fd = fd };

  i_log_info ("Server listening on port: %d\n", params.port);

  return SUCCESS;
}

err_t
server_create (server *dest, server_params params, error *e)
{
  ASSERT (dest);
  err_t ret = SUCCESS;

  ret = server_create_connectors (dest, params, e);
  if (ret)
    {
      return ret;
    }

  ret = server_connect (dest, params, e);
  if (ret)
    {
      server_free_connectors (dest);
      return ret;
    }

  return ret;
}

static inline void
server_build_pollfds (server *s)
{
  server_assert (s);
  i_memset (&s->pollfds, 0, sizeof (s->pollfds));
  s->pfdlen = 0;

  // Add (my) socket to the first index
  struct pollfd myfd = { s->fd.fd, POLLIN, 0 };
  s->pollfds[s->pfdlen++] = myfd;

  // Add all the clients to the list
  for (u32 i = 0; i < s->ccap; ++i)
    {
      if (con_is_open (&s->cons[i]))
        {
          s->pollfds[s->pfdlen++] = con_to_pollfd (&s->cons[i]);
        }
    }
}

static inline err_t
server_accept (conc_params *dest, server *s, error *e)
{
  server_assert (s);
  ASSERT (dest);

  // Accept new connector
  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof (client_addr);
  int cfd = accept (s->fd.fd, (struct sockaddr *)&client_addr, &addrlen);
  if (cfd == -1)
    {
      return error_causef (e, ERR_IO, "accept: %s", strerror (errno));
    }

  // Set to non blocking
  // I don't think this can fail
  fcntl (cfd, F_SETFL, fcntl (F_GETFL, 0) | O_NONBLOCK);

  // Create connector
  *dest = (conc_params){
    .caddrlen = addrlen,
    .cfd = (i_file){ .fd = cfd },
    .caddr = client_addr,
  };

  return SUCCESS;
}

static inline err_t
server_execute_server (server *s, error *e)
{
  server_assert (s);
  err_t ret = SUCCESS;

  // Check server socket for events
  if (s->pollfds[0].revents)
    {
      i_log_trace ("Accepting client\n");

      conc_params cparams;
      err_t_wrap (server_accept (&cparams, s, e), e);
      con_connect (&s->cons[cparams.cfd.fd], cparams);
    }

  return ret;
}

static inline void
server_execute_connectors (server *s)
{
  server_assert (s);

  for (u32 i = 1; i < s->pfdlen; ++i)
    {
      struct pollfd pfd = s->pollfds[i];
      ASSERT (pfd.fd < (int)s->ccap);

      u32 ready = pfd.revents;
      connector *con = &s->cons[pfd.fd];
      if (con_is_open (con))
        {
          if (ready & POLLIN)
            {
              ASSERT (con->want_read);
              con_read (con);
            }

          ASSERT (!con->want_close);
          con_execute (con);

          if (ready & POLLOUT)
            {
              ASSERT (con->want_write);
              con_write (con);
            }

          if (con->want_close)
            {
              con_disconnect (con);
            }
        }
    }
}

void
server_execute (server *s)
{
  server_assert (s);
  err_t ret = SUCCESS;

  // Construct the poll array
  server_build_pollfds (s);

  // Execute Poll
  int rv = poll (s->pollfds, (nfds_t)s->pfdlen, -1);
  if (rv < 0 && errno == EINTR)
    {
      // Nothing to do
      return;
    }
  if (rv < 0)
    {
      i_log_error ("Poll: %s\n", strerror (errno));
      return;
    }

  lalloc ealloc = lalloc_create (1000);
  error e = error_create (&ealloc);

  if ((ret = server_execute_server (s, &e)))
    {
      error_log_consume (&e);
      ASSERT (ealloc.used == 0);
      return;
    }

  server_execute_connectors (s);
}

void
server_close (server *s)
{
  server_assert (s);
  for (u32 i = 0; i < s->ccap; ++i)
    {
      if (con_is_open (&s->cons[i]))
        {
          con_disconnect (&s->cons[i]);
        }
    }
  lfree (s->alloc, s->cons);
}
