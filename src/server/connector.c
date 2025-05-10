#include "server/connector.h"
#include "compiler/parser.h"
#include "compiler/scanner.h"
#include "compiler/token_printer.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "vm/vm.h"

DEFINE_DBG_ASSERT_I (connector, connector, c)
{
  ASSERT (c);
}

err_t
con_create (connector *dest, con_params params, error *e)
{
  ASSERT (dest);

  scanner_params sparams = {
    .chars_input = &dest->input,
    .tokens_output = &dest->tokens,
    .string_allocator = params.alloc,
  };

  parser_params pparams = {
    .alloc = params.alloc,
    .tokens_input = &dest->tokens,
    .queries_output = &dest->queries,
  };
  parser p;
  err_t_wrap (parser_create (&p, pparams, e), e);

  vm_params vparams = {
    .queries_input = &dest->queries,
  };

  *dest = (connector){
    .cfd = (i_file){ .fd = -1 },
    .input = cbuffer_create_from (dest->_input),
    .tokens = cbuffer_create_from (dest->_tokens),
    .queries = cbuffer_create_from (dest->_queries),
    .scanner = scanner_create (sparams),
    .parser = p,
    .vm = vm_create (vparams),

    .want_read = true,
    .want_write = false,
    .want_close = false,
  };

  connector_assert (dest);
  ASSERT (!con_is_open (dest));

  return SUCCESS;
}

void
con_free (connector *c)
{
  connector_assert (c);
}

void
con_connect (connector *c, conc_params cparams)
{
  ASSERT (!con_is_open (c));
  connector_assert (c);

  ASSERT (cparams.cfd.fd >= 0);
  c->cfd = cparams.cfd;
  c->addr = cparams.caddr;
  c->addr_len = cparams.caddrlen;
}

void
con_disconnect (connector *c)
{
  connector_assert (c);
  ASSERT (con_is_open (c));

  lalloc alloc = lalloc_create (1000); // TODO - think about this
  error e = error_create (&alloc);
  err_t ret = i_close (&c->cfd, &e);
  if (ret)
    {
      error_log_consume (&e);
    }
  ASSERT (alloc.used == 0);
}

bool
con_is_open (const connector *c)
{
  connector_assert (c);
  return c->cfd.fd >= 0;
}

struct pollfd
con_to_pollfd (const connector *src)
{
  connector_assert (src);
  ASSERT (con_is_open (src));

  struct pollfd ret = { src->cfd.fd, POLLERR, 0 };
  if (src->want_write)
    {
      ret.events |= POLLOUT;
    }
  if (src->want_read)
    {
      ret.events |= POLLIN;
    }

  return ret;
}

void
con_read (connector *c)
{
  ASSERT (c);
  panic ();
}

void
con_execute (connector *c)
{
  connector_assert (c);
  ASSERT (con_is_open (c));

  lalloc ealloc = lalloc_create (1000);
  error e = error_create (&ealloc);

  if (scanner_execute (&c->scanner, &e))
    {
      error_log_consume (&e);
      c->want_close = true;
      return;
    }
  parser_execute (&c->parser);
  vm_execute (&c->vm);
}

void
con_write (connector *c)
{
  ASSERT (c);
  panic ();
}
