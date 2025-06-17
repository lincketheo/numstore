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

struct server_s
{
  i_file fd;                 // Server listening socket
  connection *cons[40];      // NULL if not connected
  struct pollfd pollfds[20]; // Poll list for connections
  u32 pfdlen;                // Length of pollfds

  database *db;
};

DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

static const char *TAG = "Server";

static inline err_t
server_bind (server *s, u16 port, error *e)
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

server *
server_open (u16 port, const string dbname, error *e)
{
  server *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate server", TAG);
      return NULL;
    }

  if (server_bind (ret, port, e))
    {
      i_free (ret);
    }

  ret->db = db_open (dbname, e);
  if (ret->db == NULL)
    {
      err_t_log_swallow (i_close (&ret->fd, &_e), _e);
      i_free (ret);
    }

  server_assert (ret);

  return ret;
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

  connection_params params = {
    .cfd = (i_file){
        .fd = cfd,
    },
    .db = s->db,
    .caddr = client_addr,
  };

  connection *c = con_open (params, e);
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

      /**
       * TODO - figure out what to do with errors:
       * https://github.com/lincketheo/numstore/issues/13
       */
      // READ
      if (ready & POLLIN)
        {
          err_t_log_swallow (con_read (con, &e), e);
        }

      // EXECUTE
      err_t_log_swallow (con_execute (con, &e), e);

      // WRITE
      if (ready & POLLOUT)
        {
          err_t_log_swallow (con_write (con, &e), e);
        }

      /**
        if (con_is_done (con))
          {
            con_close (con);
            s->cons[pfd.fd] = NULL;
          }
         */
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

bool
server_is_done (server *s)
{
  server_assert (s);
  return false;
}

void
server_close (server *s)
{
  server_assert (s);
  for (u32 i = 0; i < 40; ++i)
    {
      if (s->cons[i])
        {
          con_close (s->cons[i]);
          s->cons[i] = NULL;
        }
    }
  err_t_log_swallow (i_close (&s->fd, &_e), _e);
  i_free (s);
}
