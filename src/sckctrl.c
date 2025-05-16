#include "sckctrl.h"
#include "ds/cbuffer.h"
#include "errors/error.h"
#include "intf/io.h"

sckctrl
sckctrl_create (
    i_file cfd,
    struct sockaddr_in addr,
    cbuffer *recv,
    cbuffer *send)
{
  sckctrl ret = {
    .cfd = cfd,
    .addr = addr,
    .recv = recv,
    .send = send,
  };
  return ret;
}

err_t
sckctrl_read (sckctrl *s, error *e)
{
  err_t_wrap (cbuffer_write_some_from_file (&s->cfd, s->recv, e), e);
  return SUCCESS;
}

err_t
sckctrl_write (sckctrl *s, error *e)
{
  err_t_wrap (cbuffer_read_some_to_file (&s->cfd, s->send, e), e);
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
