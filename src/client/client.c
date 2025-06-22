#include "client/client.h"

#include "dev/assert.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
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

  // Immediately connect
  if (client_connect (ret, ipaddr, port, e))
    {
      i_free (ret);
      return NULL;
    }

  return ret;
}

err_t
client_close (client *c, error *e)
{
  client_assert (c);
  ASSERT (c->sfd.fd >= 0);

  err_t_continue (i_close (&c->sfd, e), e);

  c->sfd = (i_file){ .fd = -1 };
  i_free (c);
  return SUCCESS;
}

err_t
client_send (client *c, const string str, error *e)
{
  client_assert (c);

  // Write the header
  u16 header = str.len + sizeof (u16);
  err_t_wrap (i_write_all (&c->sfd, &header, sizeof (u16), e), e);

  // Write the data
  err_t_wrap (i_write_all (&c->sfd, str.data, str.len, e), e);

  return SUCCESS;
}

string
client_recv (client *c, error *e)
{
  client_assert (c);

  // Recieve the header
  u16 header;
  i64 read = i_read_all (&c->sfd, &header, sizeof (u16), e);
  if (read < 0)
    {
      return (string){ 0 };
    }
  if (read < (i64)sizeof (u16))
    {
      error_causef (
          e, ERR_UNEXPECTED,
          "%s Expected 2 bytes header. Got: %lld\n",
          TAG, read);
      return (string){ 0 };
    }

  // Verify data has some content
  if (header <= sizeof (u16))
    {
      error_causef (
          e, ERR_UNEXPECTED,
          "%s Server sent back a message with 0 content", TAG);
      return (string){ 0 };
    }

  string ret = (string){
    .data = i_malloc (1, header - sizeof (u16)),
    .len = header - sizeof (u16),
  };

  if (ret.data == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "%s Failed to allocate: %zu bytes for recv buffer",
          TAG, header - sizeof (u16));
      return (string){ 0 };
    }

  read = i_read_all (&c->sfd, ret.data, ret.len, e);

  // Check for failed read
  if (read < 0)
    {
      return (string){ 0 };
    }

  // Check for less than expected
  if (read != ret.len)
    {
      error_causef (
          e, ERR_UNEXPECTED,
          "%s Failed to allocate: %zu bytes for recv buffer",
          TAG, header - sizeof (u16));
      return (string){ 0 };
    }

  return ret;
}
