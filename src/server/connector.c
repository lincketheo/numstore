#include "server/connection.h"

#include "compiler/parser.h"  // parser
#include "compiler/scanner.h" // scanner
#include "sckctrl.h"          // sckctrl
#include "stmtctrl.h"         // stmtctrl
#include "vm/vm.h"            // vm

#include "ds/cbuffer.h"   // cbuffer
#include "errors/error.h" // err_t
#include "intf/io.h"      // i_file

#include <stdlib.h>   // malloc / free
#include <sys/poll.h> // pollfd

DEFINE_DBG_ASSERT_I (connection, connection, c)
{
  ASSERT (c);
}

struct connection_s
{
  bool want_close;

  /**
   * Workers
   */
  sckctrl socket;
  scanner scanner; // Scanner to tokenize input commands
  parser parser;   // Parses tokens from the scanner
  vm vm;           // Virtual machine to execute queries

  /**
   * Shared data buffers
   */
  cbuffer input;   // Recieve Buffer
  cbuffer tokens;  // Output from scanner
  cbuffer queries; // Output from parser
  cbuffer output;  // Output data
  u8 _input[20];
  token _tokens[10];
  query _queries[10];
  u8 _output[20];

  stmtctrl ctrl; // Shared statement control
};

err_t
con_create (connection **dest, i_file cfd, struct sockaddr_in caddr, error *e)
{
  ASSERT (*dest = NULL);
  connection *ret = malloc (sizeof *ret);

  if (ret == NULL)
    {
      return error_causef (
          e, ERR_NOMEM,
          "Failed to allocate connection");
    }

  ret->want_close = false;

  ret->input = cbuffer_create_from (ret->_input);
  ret->tokens = cbuffer_create_from (ret->_tokens);
  ret->queries = cbuffer_create_from (ret->_queries);
  ret->output = cbuffer_create_from (ret->_output);

  stmtctrl_create (&ret->ctrl);

  ret->socket = sckctrl_create (
      cfd,
      caddr,
      &ret->input,
      &ret->output,
      &ret->ctrl);

  ret->scanner = scanner_create (
      &ret->input,
      &ret->tokens,
      &ret->ctrl);

  parser_create (
      &ret->parser,
      &ret->tokens,
      &ret->queries,
      &ret->ctrl);

  ret->vm = vm_create (
      &ret->queries,
      &ret->ctrl);

  connection_assert (ret);

  *dest = ret;

  return SUCCESS;
}

bool
con_is_done (connection *c)
{
  return c->want_close;
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
  switch (src->ctrl.state)
    {
    case STCTRL_EXECTUING:
      {
        ret.events |= POLLIN;
        break;
      }
    case STCTRL_ERROR:
    case STCTRL_WRITING:
      {
        ret.events |= POLLOUT;
        break;
      }
    }

  return ret;
}

void
con_read (connection *c)
{
  connection_assert (c);
  ASSERT (c->ctrl.state == STCTRL_EXECTUING);

  error e = error_create (NULL);
  if (sckctrl_read (&c->socket, &e))
    {
      error_log_consume (&e);
      c->want_close = true;
    }
}

void
con_execute (connection *c)
{
  connection_assert (c);
  ASSERT (c->ctrl.state == STCTRL_EXECTUING);

  scanner_execute (&c->scanner);
  parser_execute (&c->parser);
  vm_execute (&c->vm);
}

void
con_write (connection *c)
{
  connection_assert (c);
  ASSERT (c->ctrl.state == STCTRL_WRITING || c->ctrl.state == STCTRL_ERROR);

  error e = error_create (NULL);
  if (sckctrl_write (&c->socket, &e))
    {
      error_log_consume (&e);
      c->want_close = true;
    }
}
