#pragma once

#include "intf/io.h"          // i_file
#include "mm/lalloc.h"          // lalloc
#include "server/connector.h" // connector

#include <poll.h> // struct pollfd

typedef struct
{
  i_file fd;                 // Server listening socket
  connector *cons;           // A list of open connections
  u32 ccap;                  // Capacity of connections
  struct pollfd pollfds[20]; // Poll list for connections
  u32 pfdlen;                // Length of pollfds
  lalloc *alloc;             // Allocator for everything so far
} server;

typedef struct
{
  u16 port;      // What port to open server on
  lalloc *alloc; // Global allocator
} server_params;

/**
 * Creates a server. Retruns:
 *   - ERR_NOMEM
 *   - ERR_IO - socket, bind or listen call fails
 *   - ERR_IO - bind call fails
 */
err_t server_create (server *dest, server_params params, error *e);

/**
 * Runs through one server execution cycle
 */
void server_execute (server *s);

/**
 * Free's resources
 */
void server_close (server *s);
