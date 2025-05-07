#pragma once

#include "compiler/parser.h"
#include "compiler/scanner.h"
#include "compiler/token_printer.h"
#include "compiler/tokens.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/mm.h"
#include "query/query.h"
#include "services/services.h"
#include "variables/vmem_hashmap.h"
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
  vm vm;                   // Virtual machine to execute queries
#ifdef CONNECTOR_TOK_DEBUG
  token_printer tokp; // Prints tokens
#else
  parser parser;
#endif

} connector;

typedef struct
{
  lalloc *scanner_string_allocator;
  lalloc *type_allocator;
  lalloc *stack_allocator;
  services *services;
} con_params;

typedef struct
{
  i_file cfd;
  struct sockaddr_in caddr;
  socklen_t caddrlen;
} conc_params;

// Lifetime
/**
 * Returns:
 *   - ERR_NOMEM if the stack allocator doesn't have
 *     enough room
 */
err_t con_create (connector *dest, con_params);
void con_connect (connector *c, conc_params);
err_t con_disconnect (connector *c);

// Utils
bool con_is_open (const connector *c);
struct pollfd con_to_pollfd (const connector *src);

// Read Write Execute
err_t con_read (connector *c);
void con_execute (connector *c);
