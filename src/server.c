#include "server.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "intf/stdlib.h"
#include "utils/bounds.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
}

err_t
server_create (server *dest, server_params params)
{
  ASSERT (dest);

  // Allocate connections
  dest->cons = lmalloc (params.cons_alloc, 10 * sizeof *dest->cons);
  dest->ccap = 10;
  if (dest->cons == NULL)
    {
      return ERR_NOMEM;
    }
  dest->cons_alloc = params.cons_alloc;

  // Connect to server socket
  int fd;
  struct sockaddr_in addr;
  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      return ERR_IO;
    }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (params.port);

  // BIND
  if (bind (fd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("bind");
      close (fd);
      return ERR_IO;
    }

  // LISTEN
  if (listen (fd, 3) < 0)
    {
      perror ("listen");
      close (fd);
      return ERR_IO;
    }

  dest->fd = (i_file){ .fd = fd };
  server_assert (dest);

  i_log_info ("Server listening on port: %d\n", params.port);

  return SUCCESS;
}

void
construct_pollfds (server *s)
{
  server_assert (s);
  i_memset (&s->pollfds, 0, sizeof (s->pollfds));
  s->pfdlen = 0;

  struct pollfd myfd = { s->fd.fd, POLLIN, 0 };
  s->pollfds[s->pfdlen++] = myfd;

  for (int i = 0; i < 20; ++i)
    {
      connection c = s->cons[i];
      if (c.cfd.fd > 0)
        {
          struct pollfd pfd = { c.cfd.fd, POLLERR, 0 };
          if (c.want_read)
            {
              pfd.events |= POLLIN;
            }
          if (c.want_write)
            {
              pfd.events |= POLLOUT;
            }
          s->pollfds[s->pfdlen++] = pfd;
        }
    }
}

err_t
server_double_cons (server *s)
{
  server_assert (s);

  // Integer overflow
  // int not u32 because ccap is
  // indexed like fd
  if (!can_mul_i32 (2, (int)s->ccap))
    {
      return ERR_ARITH;
    }

  connection *cons = lrealloc (s->cons_alloc, s->cons, 2 * s->ccap);

  if (cons == NULL)
    {
      return ERR_NOMEM;
    }

  s->cons = cons;
  s->ccap = 2 * s->ccap;

  return SUCCESS;
}

err_t
con_accept (server *s)
{
  server_assert (s);

  return SUCCESS;
}

err_t
con_read (connection *c)
{
  connection_assert (c);
  i32 read = cbuffer_write_max_from_file (&c->cfd, &c->input);
  if (read < 0)
    {
      return (err_t)read;
    }
  return SUCCESS;
}

err_t
con_write (connection *c)
{
  connection_assert (c);
  i32 written = cbuffer_read_max_to_file (&c->cfd, &c->output);
  if (written < 0)
    {
      return (err_t)written;
    }
  return SUCCESS;
}

err_t
server_check_my_fd (server *s)
{
  server_assert (s);
  err_t ret = SUCCESS;

  // Check server socket for events
  if (s->pollfds[0].revents)
    {
      i_log_trace ("Accepting client\n");

      // Accept new connection
      int cfd;
      struct sockaddr_in client_addr;
      socklen_t addrlen = sizeof (client_addr);
      if ((cfd = accept (s->fd.fd, (struct sockaddr *)&client_addr, &addrlen)) == -1)
        {
          perror ("accept");
          return ERR_IO;
        }

      // Test if we need to resize
      if ((cfd > (int)s->ccap) && (ret = server_double_cons (s)))
        {
          close (cfd); // TODO - handle rejection better
          return ret;
        }

      // Set to non blocking
      fcntl (cfd, F_SETFL, fcntl (F_GETFL, 0) | O_NONBLOCK);

      // Create connection
      s->cons[cfd].cfd = (i_file){ .fd = cfd };
      s->cons[cfd].addr_len = addrlen;
      s->cons[cfd].addr = client_addr;
      s->cons[cfd].want_read = true;
      s->cons[cfd].want_write = false;
      s->cons[cfd].input = cbuffer_create (s->cons[cfd]._input, sizeof (s->cons[cfd]._input));
      s->cons[cfd].output = cbuffer_create (s->cons[cfd]._output, sizeof (s->cons[cfd]._output));
    }

  return ret;
}

void
con_execute (connection *c)
{
  connection_assert (c);

  // Copy over to output
  u32 n = cbuffer_len (&c->input);
  if (n > 0)
    {
      cbuffer_cbuffer_move (&c->output, 1, n, &c->input);
    }

  if (cbuffer_len (&c->output) > 0)
    {
      c->want_write = true;
    }
}

err_t
server_check_connections (server *s)
{
  server_assert (s);
  err_t ret = SUCCESS;

  for (u32 i = 1; i < s->pfdlen; ++i)
    {
      struct pollfd pfd = s->pollfds[i];
      ASSERT (pfd.fd < (int)s->ccap);

      u32 ready = pfd.revents;
      connection *con = &s->cons[pfd.fd];
      if (con->cfd.fd > 0)
        {
          // Read Some
          if (ready & POLLIN && (ret = con_read (con)))
            {
              return ret;
            }

          // Execute Some
          con_execute (con);

          // Write Some
          if (ready & POLLOUT && (ret = con_write (con)))
            {
              return ret;
            }
        }
    }

  return ret;
}

err_t
server_execute (server *s)
{
  server_assert (s);
  err_t ret = SUCCESS;

  // Construct the poll array
  construct_pollfds (s);

  // Execute Poll
  i_log_trace ("Caling Poll\n");
  int rv = poll (s->pollfds, (nfds_t)s->pfdlen, -1);
  if (rv < 0 && errno == EINTR)
    {
      // Nothing to do
      i_log_trace ("Server has nothing to do\n");
      return SUCCESS;
    }
  if (rv < 0)
    {
      perror ("poll");
      return ERR_IO;
    }

  if ((ret = server_check_my_fd (s)))
    {
      return ret;
    }

  if ((ret = server_check_connections (s)))
    {
      return ret;
    }

  return SUCCESS;
}

void
server_close (server *s)
{
  server_assert (s);
  for (u32 i = 0; i < s->ccap; ++i)
    {
      connection c = s->cons[i];
      if (c.cfd.fd < 0)
        {
          continue;
        }
      i_close (&c.cfd);
    }
  i_close (&s->fd);
  lfree (s->cons_alloc, s->cons);
}
