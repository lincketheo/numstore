#pragma once

#include "intf/io.h"
#include "intf/mm.h"
#include "server/connector.h"

#include <poll.h>

typedef struct
{
  i_file fd;

  // A list of connectors
  // indexed by fd
  connector *cons;
  u32 ccap;

  // Polls for sockets
  struct pollfd pollfds[20];
  u32 pfdlen;

  lalloc *cons_alloc;
} server;

typedef struct
{
  u16 port;
  lalloc *cons_alloc;
} server_params;

err_t server_create (server *dest, server_params params);

err_t server_execute (server *s);

void server_close (server *s);
