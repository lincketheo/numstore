#pragma once

#include "ast/query/qspace_provider.h" // qspce_prvdr
#include "database.h"
#include "errors/error.h" // error
#include "intf/io.h"      // i_file

#include <netinet/in.h> // sockaddr_in

typedef struct connection_s connection;

// Lifecycle
connection *con_create (
    i_file cfd,
    struct sockaddr_in caddr,
    database *db,
    error *e);
bool con_is_done (connection *c);
void con_free (connection *c);

// Convert this connection to a pollfd
struct pollfd con_to_pollfd (const connection *src);

// Main methods
err_t con_read (connection *c, error *e);
err_t con_execute (connection *c, error *e);
err_t con_write (connection *c, error *e);
