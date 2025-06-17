#include "intf/stdlib.h"
#include "server/connection.h"

#include "compiler/compiler.h" // scanner
#include "database.h"          // database
#include "ds/cbuffer.h"        // cbuffer
#include "errors/error.h"      // err_t
#include "intf/io.h"           // i_file
#include "sckctrl.h"           // sckctrl
#include "virtual_machine.h"   // vm

#include <stdlib.h>   // malloc / free
#include <sys/poll.h> // pollfd

struct connection_s
{
  enum
  {
    CX_START,   // Reading the header
    CX_READING, // Reading data
    CX_WRITING, // Writing to output socket
  } state;

  u16 pos; // Where in the current header or packet we are
  union
  {
    u8 header[CMD_HDR_LEN]; // The buffered header
    u16 len;                // The actual header
  };

  i_file cfd;              // The file descriptor of this connection
  struct sockaddr_in addr; // The address of this connection

  compiler *compiler;
  database *db;
};

DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
}

connection *
con_open (connection_params params, error *e)
{
  connection *ret = i_malloc (1, sizeof *ret);
  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "Failed to allocate connection");
      return NULL;
    }

  compiler *c = compiler_create (params.db->qspce, e);
  if (c == NULL)
    {
      i_free (ret);
      return NULL;
    }

  *ret = (connection){
    .state = CX_START,
    .pos = 0,

    // header

    .cfd = params.cfd,
    .addr = params.caddr,

    .compiler = c,
    .db = params.db,
  };

  connection_assert (ret);

  return ret;
}

void
con_close (connection *c)
{
  connection_assert (c);
  i_free (c);
}

struct pollfd
con_to_pollfd (const connection *src)
{
  connection_assert (src);

  struct pollfd ret = { src->cfd.fd, POLLERR, 0 };

  switch (src->state)
    {
    case CX_WRITING:
      {
        // ret.events |= POLLOUT;
        break;
      }
    case CX_READING:
    case CX_START:
      {
        ret.events |= POLLIN;
        break;
      }
    }

  return ret;
}

err_t
con_read (connection *c, error *e)
{
  connection_assert (c);

  ASSERT (c->state != CX_WRITING);

  cbuffer *input = compiler_get_input (c->compiler);

  // Make an upfront hungry read
  err_t_wrap (cbuffer_write_some_from_file (&c->cfd, input, e), e);

  // Read the header if needed
  if (c->state == CX_START)
    {
      ASSERT (c->pos < CMD_HDR_LEN);
      u16 toread = CMD_HDR_LEN - c->pos;

      // Read into the header
      c->pos += (u16)cbuffer_read (c->header, 1, toread, input);
      ASSERT (c->pos <= CMD_HDR_LEN);

      // Transition
      if (c->pos == CMD_HDR_LEN)
        {
          u16 len;
          i_memcpy (&len, c->header, sizeof (len));
          c->len = len;
          c->state = CX_READING;
        }
    }

  // Check read buffer
  if (c->state == CX_READING)
    {
      ASSERT (c->len > c->pos);
      u16 remaining = c->len - c->pos;

      // TODO - I think you probably want
      // to only read required amount
      if (cbuffer_len (input) > remaining)
        {
          return error_causef (
              e, ERR_INVALID_ARGUMENT,
              "Socket read more bytes "
              "than header specified");
        }

      // We read all we needed - transition to writing
      if (cbuffer_len (input) == remaining)
        {
          c->state = CX_WRITING;
          c->pos = 0;
        }
    }

  return SUCCESS;
}

err_t
con_execute (connection *c, error *e)
{
  connection_assert (c);
  (void)e;
  compiler_execute (c->compiler);
  cbuffer *output = compiler_get_output (c->compiler);

  // Execute all queries
  while (cbuffer_len (output) > 0)
    {
      compiler_result q;
      u32 read = cbuffer_read (&q, sizeof q, 1, output);
      ASSERT (read == 1);

      // err_t_wrap (query_execute (c->db->pager, &q.query, e), e);
    }

  return SUCCESS;
}

err_t
con_write (connection *c, error *e)
{
  connection_assert (c);
  ASSERT (c->state == CX_WRITING);
  (void)e;
  panic ();
  // return sckctrl_write (&c, e);
}
