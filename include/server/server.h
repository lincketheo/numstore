#pragma once

#include "intf/io.h"           // i_file
#include "mm/lalloc.h"         // lalloc
#include "server/connection.h" // connection

#include <poll.h> // struct pollfd

typedef struct
{
  i_file fd;                 // Server listening socket
  connection *cons[40];      // A list of connections (NULL if not connected)
  struct pollfd pollfds[20]; // Poll list for connections
  u32 pfdlen;                // Length of pollfds
} server;

/**
 * Creates a server. Retruns:
 *   - ERR_IO - socket, bind or listen call fails
 */
err_t server_create (server *dest, u16 port, error *e);

/**
 * Runs through one server execution cycle
 * All sub components handle their own errors
 */
void server_execute (server *s);

/**
 * Free's and disconnects stuff
 */
void server_close (server *s);
