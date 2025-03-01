#pragma once

#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>

#include "config.h"
#include "dev/assert.h"
#include "dev/errors.h"
#include "intf/io.h"
#include "intf/logging.h"
#include "intf/mm.h"
#include "paging.h"
#include "sds.h"
#include "types.h"

/**
 * The server listens and accepts clients,
 * when new clients pop online, it creates a
 * new connection (`conn`)
 */
typedef struct
{
  int fd;
} server;

/**
 * A connection is in charge of reading from a connected socket
 * and executing business logic on the backend
 */
typedef struct
{
  int cfd;
  struct sockaddr_in addr;
  socklen_t addr_len;

  // Meta information
  // about the variable
  u64 ptr0;
  u32 size;
  u32 idx0;
} conn;

static inline DEFINE_DBG_ASSERT_I (server, server, s)
{
  ASSERT (s);
}

static inline DEFINE_DBG_ASSERT_I (conn, conn, c)
{
  ASSERT (c);
}

// Sets up a server
int server_setup (server *dest);

// Checks for clients, if there is one,
// populates conn - TODO - this should be changed
int server_execute (conn *dest, server *s);

int conn_execute (conn *c);
