#pragma once

#include "compiler/scanner.h"
#include "compiler/token_printer.h"
#include "compiler/tokens.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/mm.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

typedef struct
{
  i_file cfd;
  struct sockaddr_in addr;
  socklen_t addr_len;

  cbuffer input;
  cbuffer tokens;
  cbuffer output;

  u8 _input[20];
  token _tokens[10];
  u8 _output[20];

  scanner scanner;
  token_printer tokp;
} connector;

typedef struct
{
  lalloc *scanner_string_allocator;
} con_params;

typedef struct
{
  i_file cfd;
  struct sockaddr_in caddr;
  socklen_t caddrlen;
} conc_params;

// Lifetime
void con_create (connector *dest, con_params);
void con_connect (connector *c, conc_params);
err_t con_disconnect (connector *c);

// Utils
bool con_is_open (const connector *c);
struct pollfd con_to_pollfd (const connector *src);

// Read Write Execute
err_t con_read (connector *c);
void con_execute (connector *c);
err_t con_write (connector *c);
