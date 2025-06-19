#include "dev/assert.h"
#include "server/connection.h"

#include "compiler/compiler.h" // scanner
#include "database.h"          // database
#include "ds/cbuffer.h"        // cbuffer
#include "errors/error.h"      // err_t
#include "intf/io.h"           // i_file
#include "intf/stdlib.h"       // i_memcpy

#include <stdlib.h>   // malloc / free
#include <sys/poll.h> // pollfd

/**
 * A connection is in three states:
 * 1. Reading the header (tells the number of bytes to expect)
 * 2. Reading bytes
 * 3. Writing out data
 */
struct connection_s
{
  enum
  {
    CX_READ_START,  // Reading the header for how much to read
    CX_READING,     // Reading data
    CX_WRITE_START, // Reading the header for how much to write
    CX_WRITING,     // Writing to output socket
  } state;

  // The header and meta information for read / write
  struct
  {
    u16 pos; // Where in the current header or packet we are
    union
    {
      u8 header[sizeof (u16)]; // The buffered header
      u16 len;                 // The actual header
    };
  };

  i_file cfd;              // The file descriptor of this connection
  struct sockaddr_in addr; // The address of this connection

  // Reads directly into the compiler cbuffer
  compiler *compiler;

  // The database to execute on
  database *db;

  // The output that the vm writes to
  cbuffer *output; // TODO - will probably be in the vm
};

DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
  ASSERT (c->compiler);
  ASSERT (c->db);

  switch (c->state)
    {
    case CX_WRITE_START:
    case CX_READ_START:
      {
        ASSERT (c->pos < sizeof (u16));
        break;
      }
    case CX_WRITING:
    case CX_READING:
      {
        ASSERT (c->pos < c->len);
        break;
      }
    default:
      {
        UNREACHABLE ();
      }
    }
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
    .state = CX_READ_START,
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
    case CX_WRITE_START:
      {
        ret.events |= POLLOUT;
        break;
      }
    case CX_READING:
    case CX_READ_START:
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

  ASSERT (c->state == CX_READING || c->state == CX_READ_START);

  cbuffer *input = compiler_get_input (c->compiler);

  // Make an upfront hungry read
  i32 nread = cbuffer_write_some_from_file (&c->cfd, input, e);
  if (nread < 0)
    {
      return (err_t)nread;
    }

  // Read the header if needed
  if (c->state == CX_READ_START)
    {
      ASSERT (c->pos < sizeof (u16));
      u16 toread = sizeof (u16) - c->pos;

      // Read into the header
      c->pos += (u16)cbuffer_read (c->header, 1, toread, input);
      ASSERT (c->pos <= sizeof (u16));

      // Transition
      if (c->pos == sizeof (u16))
        {
          u16 len;
          i_memcpy (&len, c->header, sizeof (len)); // TODO - network endianness
          c->len = len;
          c->state = CX_READING;
        }
    }

  // Check read buffer
  if (c->state == CX_READING)
    {
      ASSERT (c->len > c->pos);
      u16 remaining = c->len - c->pos;

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
          c->state = CX_WRITE_START;
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

  i_log_info ("%d\n", cbuffer_len (output));

  // Execute all queries
  while (cbuffer_len (output) > 0)
    {
      query q;
      u32 read = cbuffer_read (&q, sizeof q, 1, output);
      ASSERT (read == 1);

      if (!q.ok)
        {
          error_log_consume (&q.e);
          panic ();
        }
    }

  return SUCCESS;
}

err_t
con_write (connection *c, error *e)
{
  connection_assert (c);

  ASSERT (c->state == CX_WRITING || c->state == CX_WRITE_START);

  cbuffer *output = c->output;

  // Read the header if needed
  if (c->state == CX_WRITE_START)
    {
      ASSERT (c->pos < sizeof (u16));
      u16 toread = sizeof (u16) - c->pos;

      // Read into the header
      c->pos += (u16)cbuffer_read (c->header, 1, toread, output);
      ASSERT (c->pos <= sizeof (u16));

      // Transition
      if (c->pos == sizeof (u16))
        {
          u16 len;
          i_memcpy (&len, c->header, sizeof (len)); // TODO - network endianness
          c->len = len;
          c->state = CX_WRITING;
        }
    }

  // Check read buffer
  if (c->state == CX_WRITING)
    {
      // TODO - you can clean this up when your brain is fresh
      ASSERT (c->len > c->pos);
      u16 remaining = c->len - c->pos;

      // Write out to the socket
      i32 nwritten = cbuffer_read_some_to_file (&c->cfd, output, e);
      if (nwritten < 0)
        {
          return (err_t)nwritten;
        }

      /**
       * TODO - I made this an assertion because it's
       * controlled by me, but should it be?
       */
      ASSERT (nwritten <= remaining);
      ASSERT (nwritten <= c->pos);
      c->pos -= nwritten;

      // We read all we needed - transition to writing
      if (nwritten == remaining)
        {
          c->state = CX_READ_START;
          c->pos = 0;
        }
    }

  return SUCCESS;
}
