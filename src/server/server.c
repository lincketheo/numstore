#include "server/server.h"
#include "dev/errors.h"
#include "intf/stdlib.h"
#include "server/connector.h"
#include "utils/bounds.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

err_t
server_create (server *dest, server_params params)
{
  ASSERT (dest);

  // Allocate connectors
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
      i_perror ("socket");
      return ERR_IO;
    }
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (params.port);

  // BIND
  if (bind (fd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      i_perror ("bind");
      close (fd);
      return ERR_IO;
    }

  // LISTEN
  if (listen (fd, 3) < 0)
    {
      i_perror ("listen");
      close (fd);
      return ERR_IO;
    }

  // Create connectors
  for (u32 i = 0; i < dest->ccap; ++i)
    {
      con_params cparams = {
        .scanner_string_allocator = dest->cons_alloc,
      };
      con_create (&dest->cons[i], cparams);
    }

  dest->fd = (i_file){ .fd = fd };
  server_assert (dest);

  i_log_info ("Server listening on port: %d\n", params.port);

  return SUCCESS;
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
server_accept (conc_params *dest, server *s)
{
  server_assert (s);
  ASSERT (dest);

  // Accept new connector
  int cfd;
  struct sockaddr_in client_addr;
  socklen_t addrlen = sizeof (client_addr);
  if ((cfd = accept (s->fd.fd, (struct sockaddr *)&client_addr, &addrlen)) == -1)
    {
      i_perror ("accept");
      return ERR_IO;
    }

  // Set to non blocking
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
server_execute_server (server *s)
{
  server_assert (s);
  err_t ret = SUCCESS;

  // Check server socket for events
  if (s->pollfds[0].revents)
    {
      i_log_trace ("Accepting client\n");
      conc_params cparams;
      if ((ret = server_accept (&cparams, s)))
        {
          return ret;
        }
      con_connect (&s->cons[cparams.cfd.fd], cparams);
    }

  return ret;
}

err_t
server_execute_connectors (server *s)
{
  server_assert (s);
  err_t ret = SUCCESS;

  for (u32 i = 1; i < s->pfdlen; ++i)
    {
      struct pollfd pfd = s->pollfds[i];
      ASSERT (pfd.fd < (int)s->ccap);

      u32 ready = pfd.revents;
      connector *con = &s->cons[pfd.fd];
      if (con_is_open (con))
        {
          // Read Some
          if (ready & POLLIN && (ret = con_read (con)))
            {
              return ret;
            }

          // Execute Some
          con_execute (con);

          // Write Some
          /*
              if (ready & POLLOUT && (ret = con_write (con)))
                {
                  return ret;
                }
        */
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
  server_build_pollfds (s);

  // Execute Poll
  int rv = poll (s->pollfds, (nfds_t)s->pfdlen, -1);
  if (rv < 0 && errno == EINTR)
    {
      // Nothing to do
      return SUCCESS;
    }
  if (rv < 0)
    {
      i_perror ("poll");
      return ERR_IO;
    }

  if ((ret = server_execute_server (s)))
    {
      return ret;
    }

  if ((ret = server_execute_connectors (s)))
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
      if (con_is_open (&s->cons[i]))
        {
          con_disconnect (&s->cons[i]);
        }
    }
  lfree (s->cons_alloc, s->cons);
}
