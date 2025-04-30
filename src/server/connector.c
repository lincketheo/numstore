#include "server/connector.h"
#include "compiler/scanner.h"
#include "compiler/token_printer.h"
#include "ds/cbuffer.h"

DEFINE_DBG_ASSERT_I (connector, connector, c)
{
  ASSERT (c);
}

void
con_create (connector *dest, con_params params)
{
  ASSERT (dest);

  dest->cfd = (i_file){ .fd = -1 };

  // Create buffers (careful about these pointer locations)
  dest->input = cbuffer_create (
      dest->_input, sizeof (dest->_input));
  dest->tokens = cbuffer_create (
      (u8 *)dest->_tokens, sizeof (dest->_tokens));
  dest->output = cbuffer_create (
      dest->_output, sizeof (dest->_output));

  // Create the scanner
  scanner_params sparams = {
    .chars_input = &dest->input,
    .tokens_output = &dest->tokens,
    .string_allocator = params.scanner_string_allocator,
  };
  scanner_create (&dest->scanner, sparams);

  // Create the token printer
  tokp_params tparams = {
    .tokens_inputs = &dest->tokens,
    .chars_output = &dest->output,
    .strings_deallocator = params.scanner_string_allocator,
  };
  tokp_create (&dest->tokp, tparams);

  connector_assert (dest);
  ASSERT (!con_is_open (dest));
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

err_t
con_disconnect (connector *c)
{
  connector_assert (c);
  ASSERT (con_is_open (c));

  err_t ret = i_close (&c->cfd);
  c->cfd.fd = -1;

  return ret;
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
  ret.events |= POLLIN;
  ret.events |= POLLOUT;

  return ret;
}

err_t
con_read (connector *c)
{
  connector_assert (c);
  ASSERT (con_is_open (c));

  i32 read = cbuffer_write_some_from_file (&c->cfd, &c->input);
  if (read < 0)
    {
      return (err_t)read;
    }

  return SUCCESS;
}

void
con_execute (connector *c)
{
  connector_assert (c);
  ASSERT (con_is_open (c));

  scanner_execute (&c->scanner);
  tokp_execute (&c->tokp);
}

err_t
con_write (connector *c)
{
  connector_assert (c);
  ASSERT (con_is_open (c));

  i32 written = cbuffer_read_some_to_file (&c->cfd, &c->output);
  if (written < 0)
    {
      return (err_t)written;
    }

  return SUCCESS;
}
