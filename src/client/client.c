#include "client/client.h"

#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/stdlib.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

struct client_s
{
  i_file sfd;                     // -1 if disconnected
  struct sockaddr_in server_addr; // meaningless if disconnected

  cbuffer send;
  cbuffer recv;

  u8 _send[10];
  u8 _recv[10];
};

DEFINE_DBG_ASSERT_I (client, client, c)
{
  ASSERT (c);
}

static const char *TAG = "Client";

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
          "%s socket: %s",
          TAG, strerror (errno));
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
          "%s inet_pton: %s",
          TAG, strerror (errno));
    }

  // Connect
  if (connect (sock, (struct sockaddr *)&saddr, sizeof (saddr)) < 0)
    {
      close (sock);
      return error_causef (
          e, ERR_IO,
          "%s Connect (%s:%d): %s",
          TAG, ipaddr, port, strerror (errno));
    }

  c->server_addr = saddr;
  c->sfd = (i_file){ .fd = sock };

  return SUCCESS;
}

client *
client_open (const char *ipaddr, u16 port, error *e)
{
  client *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (e, ERR_NOMEM, "%s Failed to allocate client", TAG);
      return NULL;
    }

  // Initialize internal buffers
  ret->sfd = (i_file){ .fd = -1 };
  ret->send = cbuffer_create_from (ret->_send);
  ret->recv = cbuffer_create_from (ret->_recv);

  // Immediately connect
  if (client_connect (ret, ipaddr, port, e))
    {
      i_free (ret);
      return NULL;
    }

  return ret;
}

void
client_close (client *c)
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

static err_t
client_recv_some (client *c, error *e)
{
  client_assert (c);
  i32 read = cbuffer_write_some_from_file (&c->sfd, &c->recv, e);
  if (read < 0)
    {
      return err_t_from (e);
    }
  return SUCCESS;
}

string
client_recv (client *c, error *e)
{
  client_assert (c);

  u16 header;
  u32 read = 0;
  while (cbuffer_len (&c->recv) < sizeof (header))
    {
      if (client_recv_some (c, e))
        {
          return (string){
            .len = 0,
            .data = NULL,
          };
        }
    }

  read = cbuffer_read (&header, 2, 1, &c->recv);
  ASSERT (read == 1);

  char *ret = i_malloc (1, header);
  if (ret == NULL)
    {
      return (string){
        .len = 0,
        .data = NULL,
      };
    }

  for (u32 i = sizeof (header); i < header; i += read)
    {
      u32 toread = header - i;
      char *offset = ret + (i - sizeof (header));
      read = cbuffer_read (offset, 1, toread, &c->recv);
    }

  return (string){
    .data = ret,
    .len = header - sizeof (header),
  };
}
