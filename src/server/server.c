#include "server/server.h"

#include "database.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/stdlib.h"
#include "server/connection.h"

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

  // 1 Database per server
  database *db;
};

DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

static const char *TAG = "Server";

/**
 * does the standard:
 *  - socket
 *  - bind
 *  - listen
 * subroutine for s->fd
 */
static inline err_t
server_init_fd (server *s, u16 port, error *e)
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

  // Allow socket to be reused in less than the timeout period
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
  // Allocate the server struct
  server *ret = i_calloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate server", TAG);
      return NULL;
    }

  // create/bind/listen
  if (server_init_fd (ret, port, e))
    {
      i_free (ret);
      return NULL;
    }

  // Open a new database
  ret->db = db_open (dbname, e);
  if (ret->db == NULL)
    {
      err_t_log_swallow (i_close (&ret->fd, &_e), _e);
      i_free (ret);
      return NULL;
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

  if (cfd > 40)
    {
      return error_causef (
          e, ERR_NOMEM,
          "%s Cfd: %d > 40, refusing to accept this many clients\n",
          TAG, cfd);
    }

  /**
   * Set to non blocking
   * I don't think this can fail
   */
  fcntl (cfd, F_SETFL, fcntl (cfd, F_GETFL, 0) | O_NONBLOCK);

  connection_params params = {
    .cfd = (i_file){
        .fd = cfd,
    },
    .db = s->db,
    .caddr = client_addr,
  };

  /**
   * NOTE: Server was the one that opened the socket
   * connection closes it. This isn't ideal. Maybe rethink
   */
  // Open a new connection
  i_log_error ("OPENING: %d\n", cfd);
  connection *c = con_open (params, e);
  if (c == NULL)
    {
      return err_t_from (e);
    }
  s->cons[cfd] = c;

  return SUCCESS;
}

static inline void
check_server_pollfd (server *s)
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

  /**
   * Iterate through each poll fd connection
   *
   * Note that pollfds is densly packed, so any
   * pfd in pollfds is necessarily "active". You can
   * get the connection associated with that pollfd
   * by indexing into cons[fd] where fd is the fd associated
   * with the struct pollfd
   */
  for (u32 i = 1; i < s->pfdlen; ++i)
    {
      struct pollfd pfd = s->pollfds[i];
      ASSERT (pfd.fd < 40);

      u32 ready = pfd.revents;
      connection *con = s->cons[pfd.fd];
      ASSERT (con);

      /**
       * Read the maximum amount that the connection can handle
       */
      if (ready & POLLIN)
        {
          err_t_log_swallow (con_read_max (con, &e), e);
        }

      /**
       * Fully execute everything that was just read
       * Note, if you don't fully execute everything, and
       * keep this part way done, you could risk causing a
       * block. Less risk because most likely you execute
       * at least 1 char so next poll will read more
       */
      con_execute_all (con);

      // WRITE
      if (ready & POLLOUT)
        {
          err_t_log_swallow (con_write_max (con, &e), e);
        }

      // Check for close - TODO
      if (false) // con_is_done (con))
        {
          err_t_log_swallow (con_close (con, &e), e);
          s->cons[pfd.fd] = NULL;
        }
    }
}

void
server_execute (server *s)
{
  server_assert (s);

  /**
   * Create the struct pollfd data structures by iterating through
   * the connection array and converting them to struct pollfd (if they are present)
   */
  {
    i_memset (&s->pollfds, 0, sizeof (s->pollfds));
    s->pfdlen = 0;

    // The first pollfd in the array is the server socket fd for calls to accept
    struct pollfd myfd = { s->fd.fd, POLLIN, 0 };
    s->pollfds[s->pfdlen++] = myfd;

    // The remaining pollfd's are all the open connections
    for (u32 i = 0; i < 40; ++i)
      {
        if (s->cons[i])
          {
            s->pollfds[s->pfdlen++] = con_to_pollfd (s->cons[i]);
          }
      }
  }

  // Execute Poll
  {
    i_log_trace ("Calling poll...\n");
    int rv = poll (s->pollfds, (nfds_t)s->pfdlen, -1);
    i_log_trace ("Poll returned: %d\n", rv);
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
  }

  // Execute business logic
  {
    // Listens for connections
    check_server_pollfd (s);

    // Executes code
    server_execute_connections (s);
  }
}

bool
server_is_done (server *s)
{
  server_assert (s);
  // TODO - I haven't figured out the logic for this function
  // Saving this for later when everything else works
  return false;
}

err_t
server_close (server *s, error *e)
{
  server_assert (s);
  err_t ret = SUCCESS;

  for (u32 i = 0; i < 40; ++i)
    {
      if (s->cons[i])
        {
          i_log_error ("CLOSING: %d\n", i);
          err_t_continue (con_close (s->cons[i], e), e);
          s->cons[i] = NULL;
        }
    }

  // Close the server file descriptor
  err_t_continue (i_close (&s->fd, e), e);

  // Close the database
  err_t_continue (db_close (s->db, e), e);
  i_free (s);

  return ret;
}
