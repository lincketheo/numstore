#pragma once

#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include "buffer.h"
#include "config.h"
#include "core/numbers.h"
#include "core/string.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "os/io.h"
#include "os/logging.h"
#include "os/mm.h"
#include "paging.h"

typedef struct
{
  int fd;
} server;

static inline DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

typedef struct
{
  int cfd;
  struct sockaddr_in addr;
  socklen_t addr_len;
  u8 *page;
} connection;

static inline DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
}

static inline int
server_setup (server *dest)
{
  ASSERT (dest);

  int fd;
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof (addr);

  // CREATE
  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      i_log_error ("Failed to create socket\n");
      return ERR_IO;
    }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (12347);

  // BIND
  if (bind (fd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("bind");
      i_log_error ("Failed to bind socket\n");
      close (fd);
      return ERR_IO;
    }

  // LISTEN
  if (listen (fd, 3) < 0)
    {
      perror ("listen");
      i_log_error ("Failed to listen on socket\n");
      close (fd);
      return ERR_IO;
    }

  dest->fd = fd;
  server_assert (dest);
  i_log_info ("Server is listening on port %d...\n", 12347);
  return SUCCESS;
}

static file_pager fpgr;
static u64 ptr0 = 0;

static inline int
server_execute (connection *dest, server *s)
{
  server_assert (s);

  int cfd;
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof (addr);
  if ((cfd = accept (s->fd, (struct sockaddr *)&addr, &addr_len)) == -1)
    {
      perror ("accept");
      i_log_error ("Failed to accept on server socket\n");
      return ERR_IO;
    }

  i_file *fp = i_open (cstrfrom ("out.bin"), 1, 1);
  fpgr.f = fp;

  dest->cfd = cfd;
  dest->addr_len = addr_len;
  dest->addr = addr;
  dest->page = (u8 *)i_malloc (app_config.page_size);
  if (dest->page == NULL)
    {
      close (dest->cfd);
      i_log_warn ("Failed to allocate client page\n");
      return ERR_IO;
    }
  connection_assert (dest);
  i_log_info ("New client connection\n");

  return SUCCESS;
}

static inline int
connection_write_execute (connection *c)
{
  connection_assert (c);

  data_list dl;
  dl_read_and_init_page (&dl, c->page);

  u64 ptr;
  int first = 1;
  u64 _read = 0;

  while (_read < 2048)
    {
      fpgr_new (&fpgr, &ptr);
      if (!first)
        {
          *dl.next = ptr;
          fpgr_commit (&fpgr, dl.data.data, ptr); // Commit previous one
          first = 0;
        }

      u32 next = MIN (dl.data.cap / sizeof (int), 2048 - _read);

      // Read from socket
      i64 nread = read (c->cfd, dl.data.data, next * sizeof (int));
      if (nread < 0)
        {
          perror ("read");
          i_log_error ("Failed to call read in connection execute\n");
          return (int)ERR_IO;
        }
      ASSERT ((u64)nread == next * sizeof (int));

      _read += nread / sizeof (int);

      // Set size to actual size
      *dl.len_num = nread / sizeof (int);
      *dl.len_denom = 1;
    }

  *dl.next = 0; // Temporary
  fpgr_commit (&fpgr, dl.data.data, ptr);

  return SUCCESS;
}

static inline int
connection_read_execute (connection *c)
{
  connection_assert (c);

  data_list dl;
  u64 ptr = 0;

  while (1)
    {
      fpgr_get (&fpgr, c->page, ptr);
      dl_read_page (&dl, c->page);

      send (c->cfd, dl.data.data, *dl.len_num * sizeof (int), 0);

      if (*dl.next == 0)
        {
          break;
        }
      ptr = *dl.next;
    }

  i_close (fpgr.f);

  return SUCCESS;
}
