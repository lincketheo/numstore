#pragma once

#include "ds/cbuffer.h"
#include <netinet/in.h>

typedef struct
{
  i_file cfd;
  struct sockaddr_in addr;

  cbuffer *recv;
  cbuffer *send;
} sckctrl;

sckctrl sckctrl_create (
    i_file cfd,
    struct sockaddr_in addr,
    cbuffer *recv,
    cbuffer *send);

err_t sckctrl_read (sckctrl *s, error *e);
err_t sckctrl_write (sckctrl *s, error *e);
void sckctrl_close (sckctrl *s);
