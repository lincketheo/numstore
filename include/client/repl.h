#pragma once

#include "client/client.h" // client
#include "errors/error.h"  // error
#include "intf/types.h"    // u32

#include <netinet/in.h> // sockaddr_in

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

err_t repl_read (repl *r, error *e);

err_t repl_execute (repl *r, error *e);
