#pragma once

#include "client/client.h"     // client
#include "core/errors/error.h" // error
#include "core/intf/types.h"   // u32

#include <netinet/in.h> // sockaddr_in

typedef struct repl_s repl;

typedef struct
{
  const char *ip_addr;
  const u16 port;
} repl_params;

repl *repl_open (repl_params params, error *e);

bool repl_is_running (repl *r);

err_t repl_execute (repl *r, error *e);

err_t repl_close (repl *r, error *e);
