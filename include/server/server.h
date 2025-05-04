#pragma once

#include "intf/io.h"
#include "intf/mm.h"
#include "server/connector.h"
#include "services/services.h"
#include "vhash_map.h"

#include <poll.h>

typedef struct
{
  // My socket
  i_file fd;

  // A list of connectors indexed by fd
  connector *cons;
  u32 ccap;

  // Polls for sockets
  struct pollfd pollfds[20];
  u32 pfdlen;

  // Services for passing to connectors
  services services;

  // Global allocator for now
  lalloc *alloc;
} server;

typedef struct
{
  u16 port;
  lalloc *alloc;
  services services;
} server_params;

err_t server_create (server *dest, server_params params);

err_t server_execute (server *s);

void server_close (server *s);
