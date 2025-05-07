#pragma once

#include "intf/io.h"
#include "intf/mm.h"
#include "server/connector.h"
#include "services/services.h"
#include "variables/vmem_hashmap.h"

#include <poll.h>

typedef struct
{
  i_file fd;                 // Server listening socket
  connector *cons;           // A list of open connections
  u32 ccap;                  // Capacity of connections
  struct pollfd pollfds[20]; // Poll list for connections
  u32 pfdlen;                // Length of pollfds
  services services;         // Available services
  lalloc *alloc;             // Allocator for everything so far
} server;

typedef struct
{
  u16 port;          // What port to open server on
  lalloc *alloc;     // Global allocator
  services services; // Available services
} server_params;

/**
 * Creates a server. Retruns:
 *   - ERR_NOMEM
 */
err_t server_create (server *dest, server_params params);

/**
 * Runs through one server execution cycle
 */
err_t server_execute (server *s);

/**
 * Free's resources
 */
void server_close (server *s);
