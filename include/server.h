#pragma once

#include "dev/errors.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/mm.h"
#include "intf/types.h"

#include <arpa/inet.h>
#include <poll.h>

typedef struct
{
  i_file cfd;
  struct sockaddr_in addr;
  socklen_t addr_len;

  bool want_read;
  bool want_write;
  bool want_close;

  cbuffer input;
  cbuffer output;

  u8 _input[10];
  u8 _output[10];
} connection;

typedef struct
{
  i_file fd;

  // A list of connections
  // indexed by fd
  connection *cons;
  u32 ccap;

  // Polls for sockets
  struct pollfd pollfds[20];
  u32 pfdlen;

  lalloc *cons_alloc;
} server;

typedef struct
{
  u16 port;
  lalloc *cons_alloc;
} server_params;

err_t server_create (server *dest, server_params params);

err_t server_execute (server *s);

void server_close (server *s);
