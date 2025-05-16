#pragma once

#include "errors/error.h"
#include "intf/io.h"

#include <netinet/in.h>

typedef struct connection_s connection;

// Lifecycle
connection *con_create (i_file cfd, struct sockaddr_in caddr);
bool con_is_done (connection *c);
void con_free (connection *c);

// Convert this connection to a pollfd
struct pollfd con_to_pollfd (const connection *src);

// Main methods
void con_read (connection *c);
void con_execute (connection *c);
void con_write (connection *c);
