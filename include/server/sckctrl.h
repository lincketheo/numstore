#pragma once

#include "core/ds/cbuffer.h"
#include <netinet/in.h>

typedef struct
{
  i_file cfd;
  struct sockaddr_in addr;

  cbuffer recv;
  cbuffer send;

  u8 _recv[10];
  u8 _send[10];
} sckctrl;

void sckctrl_create (sckctrl *dest, i_file cfd, struct sockaddr_in addr);
err_t sckctrl_read (sckctrl *s, error *e);
err_t sckctrl_write (sckctrl *s, error *e);
void sckctrl_close (sckctrl *s);
