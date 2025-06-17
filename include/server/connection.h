#pragma once

#include "ast/query/query_provider.h" // qspce_prvdr
#include "compiler/compiler.h"        // compiler
#include "database.h"                 // database
#include "errors/error.h"             // error
#include "intf/io.h"                  // i_file

#include <netinet/in.h> // sockaddr_in

#define CMD_HDR_LEN 2

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

void con_close (connection *c);

// Convert this connection to a pollfd
struct pollfd con_to_pollfd (const connection *src);

// Main methods
err_t con_read (connection *c, error *e);
err_t con_execute (connection *c, error *e);
err_t con_write (connection *c, error *e);
