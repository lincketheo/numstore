#include "server/server.h"
#include "database.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/stdlib.h"
#include "server/connection.h"
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
server_connect (server *s, u16 port, error *e)
{
  int fd;
  struct sockaddr_in addr;
  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      return error_causef (e, ERR_IO, "socket: %s", strerror (errno));
    }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (port);

  /**
   * Allow socket to be
   */
  int prop = 1;
  setsockopt (fd, SOL_SOCKET, SO_REUSEADDR, &prop, sizeof (prop));

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

  i_log_info ("Server listening on port: %d\n", port);

  return SUCCESS;
}

err_t
server_create (
    server *dest,
    u16 port,
    const string dbname,
    error *e)
{
  ASSERT (dest);

  err_t_wrap (server_connect (dest, port, e), e);

  if (!i_exists_rw (dbname))
    {
      err_t_wrap (db_create (dbname, e), e);
    }
  err_t_wrap (db_open (&dest->db, dbname, e), e);

  return SUCCESS;
}

static inline err_t
server_accept (server *s, error *e)
{
  server_assert (s);

  // Accept new connection
  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof (client_addr);
  int cfd = accept (s->fd.fd, (struct sockaddr *)&client_addr, &addrlen);
  if (cfd == -1)
    {
      return error_causef (e, ERR_IO, "accept: %s", strerror (errno));
    }

  /**
   * Set to non blocking
   * I don't think this can fail
   */
  fcntl (cfd, F_SETFL, fcntl (F_GETFL, 0) | O_NONBLOCK);

  connection *c = con_create (
      (i_file){ .fd = cfd },
      client_addr,
      &s->db, e);

  if (c == NULL)
    {
      return err_t_from (e);
    }
  s->cons[cfd] = c;

  return SUCCESS;
}

static inline void
server_execute_server (server *s)
{
  server_assert (s);

  // Check server socket for events
  if (s->pollfds[0].revents)
    {
      i_log_trace ("Accepting client\n");
      error e = error_create (NULL);
      if (server_accept (s, &e))
        {
          error_log_consume (&e);
          return;
        }
    }
}

static inline void
server_execute_connections (server *s)
{
  server_assert (s);

  for (u32 i = 1; i < s->pfdlen; ++i)
    {
      struct pollfd pfd = s->pollfds[i];
      ASSERT (pfd.fd < 40);

      u32 ready = pfd.revents;
      connection *con = s->cons[pfd.fd];
      ASSERT (con);

      // READ
      if (ready & POLLIN)
        {
          con_read (con);
        }

      // EXECUTE
      con_execute (con);

      // WRITE
      if (ready & POLLOUT)
        {
          con_write (con);
        }

      if (con_is_done (con))
        {
          con_free (con);
          s->cons[pfd.fd] = NULL;
        }
    }
}

void
server_execute (server *s)
{
  server_assert (s);

  // Create poll fd's
  {
    i_memset (&s->pollfds, 0, sizeof (s->pollfds));
    s->pfdlen = 0;

    // Add (my) socket to the first index
    struct pollfd myfd = { s->fd.fd, POLLIN, 0 };
    s->pollfds[s->pfdlen++] = myfd;

    // Add all the clients to the list
    for (u32 i = 0; i < 40; ++i)
      {
        if (s->cons[i])
          {
            s->pollfds[s->pfdlen++] = con_to_pollfd (s->cons[i]);
          }
      }
  }

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

  // Listens for connections
  server_execute_server (s);

  // Executes code
  server_execute_connections (s);
}

void
server_close (server *s)
{
  server_assert (s);
  for (u32 i = 0; i < 40; ++i)
    {
      if (s->cons[i])
        {
          con_free (s->cons[i]);
          s->cons[i] = NULL;
        }
    }
}
