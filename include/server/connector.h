#pragma once

#include "compiler/parser.h"
#include "compiler/scanner.h"
#include "compiler/token_printer.h"
#include "compiler/tokens.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "mm/lalloc.h"
#include "query/query.h"
#include "vm/vm.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

//#define CONNECTOR_TOK_DEBUG

typedef struct
{
  i_file cfd;              // File descriptor we are open on
  struct sockaddr_in addr; // Address that is connected
  socklen_t addr_len;      // Length of address - not sure if needed
  cbuffer input;           // RCV Buffer
  cbuffer tokens;          // Output from scanner
  cbuffer queries;         // Output from parser
  u8 _input[20];           // Backing for input
  token _tokens[10];       // Backing for tokens
  query _queries[10];      // Backing for queries
  scanner scanner;         // Scanner to tokenize input commands
  parser parser;           // Parses tokens from the scanner
  vm vm;                   // Virtual machine to execute queries

  /**
   * Signals back to the server
   */
  bool want_read;
  bool want_write;
  bool want_close;
} connector;

typedef struct
{
  lalloc *alloc;
} con_params;

// Lifetime
/**
 * Returns:
 *   - ERR_NOMEM if the stack allocator doesn't have
 *     enough room
 */
err_t con_create (connector *dest, con_params params, error *e);

void con_free (connector *c);

typedef struct
{
  i_file cfd;
  struct sockaddr_in caddr;
  socklen_t caddrlen;
} conc_params;

void con_connect (connector *c, conc_params p);

void con_disconnect (connector *c);

bool con_is_open (const connector *c);

struct pollfd con_to_pollfd (const connector *src);

void con_read (connector *c);

void con_execute (connector *c);

void con_write (connector *c);
