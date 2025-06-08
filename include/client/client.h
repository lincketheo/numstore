#pragma once

#include "ds/cbuffer.h" //cbuffer
#include "intf/io.h"    // i_file
#include "intf/types.h" // u16
#include "mm/lalloc.h"  // lalloc

#include <netinet/in.h> // sockaddr_in

typedef struct
{
  i_file sfd;                     // -1 if disconnected
  struct sockaddr_in server_addr; // meaningless if disconnected

  cbuffer send;
  cbuffer recv;

  u8 _send[10];
  u8 _recv[10];
} client;

/**
 * Creates a "Disconnected client" and initializes buffers.
 * Careful about copying etc, the cbuffer points to this instance's
 * _send / _recv buffers
 */
err_t client_create (
    client *dest,
    const char *ipaddr,
    u16 port,
    error *e);

/**
 * Disconnects. Client must be connected
 */
void client_disconnect (client *c);

/**
 * Writes the entire contents of string
 * (many write system calls)
 */
err_t client_send (client *c, const string str, error *e);
