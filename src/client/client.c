#include "client/client.h"
#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/stdlib.h"
#include "mm/lalloc.h"

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

static err_t
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

err_t
client_create (
    client *dest,
    const char *ipaddr,
    u16 port,
    error *e)
{
  ASSERT (dest);

  dest->sfd = (i_file){ .fd = -1 };
  dest->send = cbuffer_create_from (dest->_send);
  dest->recv = cbuffer_create_from (dest->_recv);

  return client_connect (dest, ipaddr, port, e);
}

void
client_disconnect (client *c)
{
  client_assert (c);
  ASSERT (c->sfd.fd >= 0);

  error e = error_create (NULL);
  if (i_close (&c->sfd, &e))
    {
      error_log_consume (&e);
    }

  c->sfd = (i_file){ .fd = -1 };
}

static err_t
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
client_send (client *c, const string str, error *e)
{
  client_assert (c);
  u32 written;

  /**
   * Prefix command with number of bytes
   */
  string newstr = (string){
    .data = i_malloc (2 + str.len, 1),
    .len = 2 + str.len,
  };

  if (newstr.data == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Failed to allocate command wrapper");
    }

  u16 header = newstr.len;
  i_memcpy (newstr.data, &header, 2);
  i_memcpy (newstr.data + 2, str.data, str.len);

  // Send all
  for (u32 i = 0; i < str.len; i += written)
    {
      written = cbuffer_write (
          &newstr.data[i],
          1, newstr.len - i, &c->send);

      if (client_send_some (c, e))
        {
          goto theend;
        }
    }

theend:
  i_free (newstr.data);
  return err_t_from (e);
}
