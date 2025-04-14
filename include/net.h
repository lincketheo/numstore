#pragma once

#include "core/buffer.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "os/stdlib.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

DEFINE_DYN_BUFFER (u8, u8)

typedef struct
{
  struct sockaddr_in addr;
  int fd;
  u8_buffer in;
  u8_buffer out;
} cctx;

int
cctx_create (cctx *dest, int fd, struct sockaddr_in addr)
{
  ASSERT (dest);
  ASSERT (fd >= 0);
  dest->fd = fd;
  dest->addr = addr;

  int res = u8_buffer_malloc (&dest->in, 10);
  ASSERT (res == 0);
  res = u8_buffer_malloc (&dest->out, 10);
  ASSERT (res == 0);

  return SUCCESS;
}

void
cctx_close (cctx *c)
{
  ASSERT (c);
  close (c->fd);
  u8_buffer_free (&c->in);
  u8_buffer_free (&c->out);
}

DEFINE_DYN_BUFFER (cctx, cctx)
DEFINE_DYN_BUFFER (struct pollfd, pollfd)

typedef struct
{
  struct sockaddr_in addr;
  int fd;
  cctx_buffer cnxs;
  pollfd_buffer pfds;
} sctx;

static inline sctx
sctx_open ()
{
  int res;
  sctx ret;

  // Create connection buffer
  res = cctx_buffer_malloc (&ret.cnxs, 10);
  ASSERT (res >= 0);
  res = pollfd_buffer_malloc (&ret.pfds, 10);
  ASSERT (res >= 0);

  // Create socket
  ret.fd = socket (AF_INET, SOCK_STREAM, 0);
  ASSERT (ret.fd != -1);

  // Set socket to non-blocking mode
  res = fcntl (ret.fd, F_GETFL, 0);
  ASSERT (res != -1);
  res = fcntl (ret.fd, F_SETFL, res | O_NONBLOCK);
  ASSERT (res != -1);

  // Create address
  ret.addr.sin_family = AF_INET;
  ret.addr.sin_addr.s_addr = INADDR_ANY;
  ret.addr.sin_port = htons (8080);

  // Bind
  res = bind (ret.fd, (struct sockaddr *)&ret.addr, sizeof (ret.addr));
  ASSERT (res >= 0);

  // Listen
  res = listen (ret.fd, 3);
  ASSERT (res >= 0);

  return ret;
}

static inline void
sctx_accept (sctx *s)
{
  while (1)
    {
      // Accept until you can't
      // TODO - you should add a limit here
      struct sockaddr_in c_addr;
      socklen_t addr_len = sizeof (c_addr);

      int fd = accept (s->fd, (struct sockaddr *)&c_addr, &addr_len);

      if (fd < 0)
        {
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
              break;
            }
          else
            {
              ASSERT (0);
            }
        }

      cctx c;
      c.addr = c_addr;
      c.fd = fd;
      if (s->cnxs.len == s->cnxs.cap)
        {
          int res = cctx_buffer_resize (&s->cnxs, s->cnxs.cap * 2);
          ASSERT (res >= 0);
          res = pollfd_buffer_resize (&s->pfds, s->pfds.cap * 2);
          ASSERT (res >= 0);
        }

      // Push new connection
      s->cnxs.data[s->cnxs.len++] = c;
      struct pollfd pfd = {
        .fd = fd,
        .events = POLLERR | POLLIN,
        .revents = 0,
      };
      s->pfds.data[s->pfds.len++] = pfd;
    }
}

static inline void
sctx_read_write_some (sctx *s)
{
  ASSERT (s);
  int ret = poll (s->pfds.data, s->pfds.len, 0);
  if (ret < 0 && errno == EINTR)
    {
      return;
    }
  else if (ret < 0)
    {
      ASSERT (0);
    }

  // handle connection sockets
  for (u64 i = 0; i < s->pfds.len; ++i)
    {
      short int ready = s->pfds.data[i].revents;
      cctx c = s->cnxs.data[i];
      if (ready & POLLIN)
        {
        }
      if (ready & POLLOUT)
        {
        }
    }
}

static inline void
sctx_do_work (sctx *s)
{
  ASSERT (s);
  for (u64 i = 0; i < s->cnxs.len; ++i)
    {
      // TODO - add this logic to the buffer api
      u64 inlen = s->cnxs.data[i].in.len;
      u64 incap = s->cnxs.data[i].in.cap;
      u64 olen = s->cnxs.data[i].out.len;
      u64 ocap = s->cnxs.data[i].out.cap;

      // Just copy data over
      if (olen + inlen > ocap)
        {
          int res = u8_buffer_resize (&s->cnxs.data[i].out, olen + inlen);
          ASSERT (res == 0);
        }
      i_memcpy (&s->cnxs.data[i].out.data[olen], &s->cnxs.data[i].in.data, inlen);
    }
}
