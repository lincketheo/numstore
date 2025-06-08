#pragma once

#include "ast/query/query_provider.h" // qspce_prvdr
#include "compiler/compiler.h"        // compiler
#include "database.h"                 // database
#include "errors/error.h"             // error
#include "intf/io.h"                  // i_file

#include <netinet/in.h> // sockaddr_in

#define CMD_HDR_LEN 2

typedef struct
{
  enum
  {
    CX_START,   // Reading the header
    CX_READING, // Reading data
    CX_WRITING, // Writing to output socket
  } state;

  u16 pos;
  union
  {
    u8 header[CMD_HDR_LEN]; // The buffered header
    u16 len;                // The actual header
  };

  i_file cfd;
  struct sockaddr_in addr;

  compiler compiler; // Compiles chars to AST

  database *db;
} connection;

typedef struct
{
  i_file cfd;
  struct sockaddr_in caddr;
  database *db;
} connection_params;

/**
 * Creates and Allocates a new connection
 *
 * Errors:
 *  - ERR_NOMEM - can't allocate connection
 */
connection *con_create (connection_params params, error *e);

void con_free (connection *c);

// Convert this connection to a pollfd
struct pollfd con_to_pollfd (const connection *src);

// Main methods
err_t con_read (connection *c, error *e);
err_t con_execute (connection *c, error *e);
err_t con_write (connection *c, error *e);
