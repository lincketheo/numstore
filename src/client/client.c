#include "client/client.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/mm.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

DEFINE_DBG_ASSERT_I (client, client, c)
{
  ASSERT (c);
}

void
client_create (client *dest)
{
  ASSERT (dest);
  dest->sfd = (i_file){ .fd = -1 };
  dest->send = cbuffer_create_from (dest->_send);
  dest->recv = cbuffer_create_from (dest->_recv);
  client_assert (dest);
}

err_t
client_connect (client *c, const char *ipaddr, u16 port, error *e)
{
  client_assert (c);
  ASSERT (ipaddr);

  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      return error_causef (
          e, ERR_IO,
          "socket: %s",
          strerror (errno));
    }

  // Create the ip address
  struct sockaddr_in saddr;
  saddr.sin_family = AF_INET;
  saddr.sin_port = htons (port);
  if (inet_pton (AF_INET, ipaddr, &saddr.sin_addr) <= 0)
    {
      close (sock);
      return error_causef (
          e, ERR_IO,
          "inet_pton: %s",
          strerror (errno));
    }

  // Connect
  if (connect (sock, (struct sockaddr *)&saddr, sizeof (saddr)) < 0)
    {
      close (sock);
      return error_causef (
          e, ERR_IO,
          "Connect (%s:%d): %s",
          ipaddr, port, strerror (errno));
    }

  c->server_addr = saddr;
  c->sfd = (i_file){ .fd = sock };

  return SUCCESS;
}

void
client_disconnect (client *c)
{
  client_assert (c);
  ASSERT (c->sfd.fd >= 0);

  lalloc ealloc = lalloc_create (1000);
  error e = error_create (&ealloc);
  if (i_close (&c->sfd, &e))
    {
      error_log_consume (&e);
    }
  ASSERT (ealloc.used == 0);

  c->sfd = (i_file){ .fd = -1 };
}

err_t
client_send_some (client *c, error *e)
{
  client_assert (c);
  i32 written = cbuffer_read_some_to_file (&c->sfd, &c->send, e);
  if (written < 0)
    {
      return err_t_from (e);
    }
  return SUCCESS;
}

err_t
client_recv_some (client *c, error *e)
{
  client_assert (c);
  i32 read = cbuffer_write_some_from_file (&c->sfd, &c->recv, e);
  if (read < 0)
    {
      return err_t_from (e);
    }

  if (read > 0)
    {
      i_file out = { .fd = fileno (stdout) };
      i32 written = cbuffer_read_some_to_file (&out, &c->recv, e);
      if (written < 0)
        {
          return err_t_from (e);
        }
    }
  return SUCCESS;
}

err_t
client_execute_all (client *c, const string str, error *e)
{
  client_assert (c);
  u32 written;

  // Send all
  for (u32 i = 0; i < str.len; i += written)
    {
      written = cbuffer_write (&str.data[i], 1, str.len - i, &c->send);
      err_t_wrap (client_send_some (c, e), e);

      /*
        if ((ret = client_recv_some (c)))
          {
            return ret;
          }
          */
    }

  // Flush
  /*
  for (int i = 0; i < 10; ++i)
    {
      if ((ret = client_recv_some (c)))
        {
          return ret;
        }
    }
  */

  return SUCCESS;
}
