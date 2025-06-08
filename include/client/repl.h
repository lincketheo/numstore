#pragma once

#include "client/client.h" // client
#include "errors/error.h"  // error
#include "intf/types.h"    // u32

#include <netinet/in.h> // sockaddr_in

typedef struct
{
  char *buffer; // A buffer to send over the socket
  u32 blen;     // length of buffer

  const char *ip_addr; // Ip address of server
  const u16 port;      // port of server

  client client; // On going client object

  bool running;
} repl;

typedef struct
{
  const char *ip_addr;
  const u16 port;
} repl_params;

err_t repl_create (repl *dest, repl_params params, error *e);

err_t repl_execute (repl *r, error *e);

void repl_release (repl *r);
