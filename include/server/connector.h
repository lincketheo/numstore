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
#include "vhash_map.h"
#include "vm/vm.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

//#define CONNECTOR_TOK_DEBUG

typedef struct
{
  i_file cfd;
  struct sockaddr_in addr;
  socklen_t addr_len;

  cbuffer input;
  cbuffer tokens;
  cbuffer queries;

  u8 _input[20];
  token _tokens[10];
  query _queries[10];

  scanner scanner;

#ifdef CONNECTOR_TOK_DEBUG
  token_printer tokp;
#else
  parser parser;
  vm vm;
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
