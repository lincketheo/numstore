#include "server/connection.h"

#include "compiler/compiler.h" // scanner
#include "database.h"          // database
#include "dev/assert.h"        // DEFINE_DBG_ASSERT_I
#include "ds/cbuffer.h"        // cbuffer
#include "errors/error.h"      // err_t
#include "intf/io.h"           // i_file
#include "intf/stdlib.h"       // i_memcpy
#include "virtual_machine.h"

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
  i_close (&c->cfd, e);
  i_free (c);
  return SUCCESS;
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

      compiler_execute (c->compiler);
      vm_execute (c->vm);

      // We read all we needed - transition to writing
      if (cbuffer_len (input) == remaining)
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
