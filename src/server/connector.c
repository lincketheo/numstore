#include <stdlib.h>   // malloc / free
#include <sys/poll.h> // pollfd

#include "core/dev/assert.h"   // DEFINE_DBG_ASSERT_I
#include "core/ds/cbuffer.h"   // cbuffer
#include "core/errors/error.h" // err_t
#include "core/intf/io.h"      // i_file
#include "core/intf/stdlib.h"  // i_memcpy

#include "compiler/compiler.h" // scanner

#include "server/connection.h" // TODO

#include "numstore/database.h"        // database
#include "numstore/virtual_machine.h" // TODO

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

  // Write directly out of the virtual machine
  vm *vm;
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

static const char *TAG = "Connection";

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

  vm *v = vm_open (params.db->pager, compiler_get_output (c), e);
  if (v == NULL)
    {
      i_free (ret);
      compiler_free (c);
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
    .vm = v,
  };

  connection_assert (ret);

  return ret;
}

err_t
con_close (connection *c, error *e)
{
  connection_assert (c);
  compiler_free (c->compiler);
  vm_close (c->vm);

  /**
   * NOTE: Server was the one that opened the socket
   * connection closes it. This isn't ideal. Maybe rethink
   */
  err_t_continue (i_close (&c->cfd, e), e);
  i_free (c);
  return err_t_from (e);
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
con_read_max (connection *c, error *e)
{
  connection_assert (c);

  // Validate correct state
  if (!(c->state == CX_READING || c->state == CX_READ_START))
    {
      return SUCCESS;
    }

  /**
   * TODO - prove that you can't get here without being in these states:
   * ASSERT (c->state == CX_READING || c->state == CX_READ_START);
   */

  cbuffer *input = compiler_get_input (c->compiler);

  // Make an upfront hungry read
  i32 nread = cbuffer_write_some_from_file (&c->cfd, input, e);
  if (nread < 0)
    {
      return (err_t)nread;
    }
  c->pos += nread;

  // Read the header if needed
  if (c->state == CX_READ_START)
    {
      // Read into the header
      u16 header = cbuffer_read (c->header, sizeof (u16), 1, input);
      if (header)
        {
          // Copy into actual len
          // TODO - network endianness
          i_memmove (&c->len, c->header, sizeof (c->len));
          c->state = CX_READING;
        }
    }

  // Check position
  if (c->pos > c->len)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s Input overflow on read, expected: %d bytes, got %d bytes",
          TAG, c->len, c->pos);
    }

  // Check read buffer
  if (c->state == CX_READING)
    {
      ASSERT (c->len >= c->pos);

      compiler_execute (c->compiler);
      vm_execute (c->vm);

      if (c->pos == c->len)
        {
          c->state = CX_WRITE_START;
          c->pos = 0;
        }
    }

  return SUCCESS;
}

void
con_execute_all (connection *c)
{
  connection_assert (c);

  compiler_execute (c->compiler);
  vm_execute (c->vm);
}

err_t
con_write_max (connection *c, error *e)
{
  connection_assert (c);

  if (!(c->state == CX_WRITE_START || c->state == CX_WRITING))
    {
      return SUCCESS;
    }

  /**
   * TODO - prove that you can't get here without being in these states:
   * ASSERT (c->state == CX_WRITING || c->state == CX_WRITE_START);
   */

  cbuffer *output = vm_get_output (c->vm);

  // Read the header if needed
  if (c->state == CX_WRITE_START)
    {
      // Read into the header
      u16 header = (u16)cbuffer_read (c->header, sizeof (u16), 1, output);
      if (header)
        {
          i_memmove (&c->len, c->header, sizeof (c->len));
          // Write the length to the file first
          err_t_wrap (i_write_all (&c->cfd, &c->len, sizeof (u16), e), e);
          c->pos += sizeof (u16);
          c->state = CX_WRITING;
        }
    }

  if (c->pos > c->len)
    {
      return error_causef (
          e, ERR_INVALID_ARGUMENT,
          "%s Input overflow on write, expected: %d bytes, got %d bytes",
          TAG, c->len, c->pos);
    }

  // Check read buffer
  if (c->state == CX_WRITING)
    {
      // TODO - you can clean this up when your brain is fresh
      ASSERT (c->len >= c->pos);

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
      c->pos += nwritten;
      ASSERT (c->pos <= c->len);

      // We read all we needed - transition to writing
      if (c->pos == c->len)
        {
          c->state = CX_READ_START;
          c->pos = 0;
        }
    }

  return SUCCESS;
}
