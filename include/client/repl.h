#pragma once

#include "client/client.h"
#include "dev/errors.h"
#include "intf/mm.h"
#include "intf/types.h"

typedef struct
{
  char *buffer;
  u32 blen;

  const char *ip_addr;
  const u16 port;

  client client;
} repl;

typedef struct
{
  const char *ip_addr;
  const u16 port;
} repl_params;

void repl_create (repl *dest, repl_params params);

err_t repl_read (repl *r);

err_t repl_execute (repl *r);
