#pragma once

#include <poll.h> // struct pollfd

#include "core/ds/strings.h"   // TODO
#include "core/errors/error.h" // TODO
#include "core/intf/types.h"   // TODO

typedef struct server_s server;

/**
 * Creates a server. Retruns:
 *   - ERR_IO - socket, bind or listen call fails
 *   - ERR_NOMEM - Fails to allocate server
 */
server *server_open (u16 port, const string dbname, error *e);

/**
 * Runs through one server execution cycle
 * All sub components handle their own errors
 */
void server_execute (server *s);

/**
 * If true, you musn't call
 * server_execute again
 */
bool server_is_done (server *s);

/**
 * Free's and disconnects stuff
 */
err_t server_close (server *s, error *e);
