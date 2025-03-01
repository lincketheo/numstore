#include "net.h"
#include "config.h"
#include "intf/mm.h"

static file_pager fpgr;

int
server_setup (server *dest)
{
  ASSERT (dest);

  int fd;
  struct sockaddr_in addr;

  // CREATE
  if ((fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      i_log_error ("Failed to create socket\n");
      return ERR_IO;
    }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons (33347);

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
  i_log_info ("Server is listening on port %d...\n", 33347);
  return SUCCESS;
}

int
server_execute (conn *dest, server *s)
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

  i_file *fp = i_open (unsafe_cstrfrom ("out.bin"), 1, 1);
  fpgr.f = fp;

  dest->cfd = cfd;
  dest->addr_len = addr_len;
  dest->addr = addr;

  conn_assert (dest);
  i_log_info ("New client conn\n");

  return SUCCESS;
}

/**
int
conn_execute (conn *c)
{
  conn_assert (c);

  u8 *dest = i_malloc (app_config.page_size);

  // First page - root node
  fpgr_get (&fpgr, dest, c->ptr0);

  u64 ptr0;
  u8 *page0 = i_malloc (app_config.page_size);
  ASSERT (page0);
  data_list dl0;
  dl_read_and_init_page (&dl0, page0);

  u64 ptr1;
  u8 *page1 = i_malloc (app_config.page_size);
  ASSERT (page1);
  data_list dl1;
  dl_read_and_init_page (&dl1, page1);

  u64 _read = 0;

  fpgr_new (&fpgr, &ptr0);

  while (_read < 2048)
    {
      fpgr_new (&fpgr, &nptr);
      if (!first)
        {
          *dl.next = nptr;
          fpgr_commit (&fpgr, dl.data.data, ptr); // Commit previous one
        }
      ptr = nptr;
      first = 0;

      u32 next = MIN (dl.data.cap / sizeof (int), 2048 - _read);

      // Read from socket
      i64 nread = read (c->cfd, dl.data.data, next * sizeof (int));
      if (nread < 0)
        {
          perror ("read");
          i_log_error ("Failed to call read in conn execute\n");
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
conn_read_execute (conn *c)
{
  conn_assert (c);

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
*/
