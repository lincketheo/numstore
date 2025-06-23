#include "server/sckctrl.h"
#include "core/ds/cbuffer.h"
#include "core/errors/error.h"
#include "core/intf/io.h"

void
sckctrl_create (
    sckctrl *dest,
    i_file cfd,
    struct sockaddr_in addr)
{
  dest->cfd = cfd;
  dest->addr = addr;
  dest->recv = cbuffer_create_from (dest->_recv);
  dest->send = cbuffer_create_from (dest->_send);
}

err_t
sckctrl_read (sckctrl *s, error *e)
{
  err_t_wrap (cbuffer_write_some_from_file (&s->cfd, &s->recv, e), e);
  return SUCCESS;
}

err_t
sckctrl_write (sckctrl *s, error *e)
{
  err_t_wrap (cbuffer_read_some_to_file (&s->cfd, &s->send, e), e);
  return SUCCESS;
}

void
sckctrl_close (sckctrl *s)
{
  error e = error_create (NULL);
  if (i_close (&s->cfd, &e))
    {
      error_log_consume (&e);
    }
}
