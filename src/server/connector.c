#include "intf/stdlib.h"
#include "server/connection.h"

#include "compiler/compiler.h" // compiler
#include "database.h"          // database

#include "compiler/parser.h"  // parser
#include "compiler/scanner.h" // scanner
#include "sckctrl.h"          // sckctrl

#include "ds/cbuffer.h"   // cbuffer
#include "errors/error.h" // err_t
#include "intf/io.h"      // i_file
#include "virtual_machine.h"

#include <stdlib.h>   // malloc / free
#include <sys/poll.h> // pollfd

DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
}

#define CMD_HDR_LEN 2

struct connection_s
{
  enum
  {
    CX_START,   // Reading the header
    CX_READING, // Reading data
    CX_WRITING, // Writing to output socket
  } state;

  u16 pos;
  union
  {
    u8 header[CMD_HDR_LEN]; // The buffered header
    u16 len;                // The actual header
  };

  /**
   * Workers
   */
  sckctrl socket;
  compiler compiler;

  /**
   * Shared data buffers
   */
  cbuffer input;   // Recieve Buffer
  cbuffer queries; // Output from parser

  u8 _input[20];
  query _queries[10];

  database *db;
};

connection *
con_create (
    i_file cfd,
    struct sockaddr_in caddr,
    database *db,
    error *e)
{
  connection *ret = i_malloc (1, sizeof *ret);

  if (ret == NULL)
    {
      error_causef (
          e, ERR_NOMEM,
          "Failed to allocate connection");
      return NULL;
    }

  ret->pos = 0;
  ret->state = CX_START;

  ret->socket = sckctrl_create (cfd, caddr, &ret->input, NULL);

  ret->input = cbuffer_create_from (ret->_input);
  ret->queries = cbuffer_create_from (ret->_queries);

  ret->db = db;

  compiler_create (&ret->compiler, &ret->input, &ret->queries, db->qspce);

  connection_assert (ret);

  return ret;
}

bool
con_is_done (connection *c)
{
  (void)c;
  return false;
}

void
con_free (connection *c)
{
  connection_assert (c);
}

void
con_disconnect (connection *c)
{
  connection_assert (c);
  sckctrl_close (&c->socket);
  free (c);
}

struct pollfd
con_to_pollfd (const connection *src)
{
  connection_assert (src);

  struct pollfd ret = { src->socket.cfd.fd, POLLERR, 0 };

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

  // Make an upfront hungry read
  err_t_wrap (sckctrl_read (&c->socket, e), e);

  // Read the header if needed
  if (c->state == CX_START)
    {
      ASSERT (c->pos < CMD_HDR_LEN);
      u16 toread = CMD_HDR_LEN - c->pos;

      c->pos += (u16)cbuffer_read (c->header, 1, toread, &c->input);
      ASSERT (c->pos <= CMD_HDR_LEN);

      // Transition
      if (c->pos == CMD_HDR_LEN)
        {
          u16 len;
          i_memcpy (&len, c->header, 2);
          c->len = len;
          c->state = CX_READING;
        }
    }

  // Check read buffer
  if (c->state == CX_READING)
    {
      ASSERT (c->len > c->pos);
      u16 remaining = c->len - c->pos;
      if (cbuffer_len (&c->input) > remaining)
        {
          return error_causef (
              e, ERR_INVALID_ARGUMENT,
              "Socket read more bytes "
              "than header specified");
        }

      // We read all we needed - transition to writing
      if (cbuffer_len (&c->input) == remaining)
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
  err_t_wrap (compiler_execute (&c->compiler, e), e);

  while (cbuffer_len (&c->queries) > 0)
    {
      query q;
      u32 read = cbuffer_read (&q, sizeof q, 1, &c->queries);
      ASSERT (read == 1);

      err_t_wrap (query_execute (c->db->pager, &q, e), e);
    }

  return SUCCESS;
}

err_t
con_write (connection *c, error *e)
{
  connection_assert (c);
  ASSERT (c->state == CX_WRITING);
  return sckctrl_write (&c->socket, e);
}
