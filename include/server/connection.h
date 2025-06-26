#pragma once

#include <netinet/in.h> // sockaddr_in

#include "core/errors/error.h" // error
#include "core/intf/io.h"      // i_file

#include "compiler/compiler.h" // compiler

#include "numstore/database.h"             // database
#include "numstore/query/query_provider.h" // qspce_prvdr

typedef struct connection_s connection;

typedef struct
{
  i_file cfd;
  struct sockaddr_in caddr;
  database *db;
  query_provider *qp;
} connection_params;

/**
 * Creates and Allocates a new connection
 *
 * Errors:
 *  - ERR_NOMEM - can't allocate connection
 */
connection *con_open (connection_params params, error *e);

err_t con_close (connection *c, error *e);

// Convert this connection to a pollfd
struct pollfd con_to_pollfd (const connection *src);

// Main methods
err_t con_read_max (connection *c, error *e);
void con_execute_all (connection *c);
err_t con_write_max (connection *c, error *e);
