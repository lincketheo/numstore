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
void client_create (client *dest);

/**
 * Connects. Client must be disconnected
 * Makes a new connection with the ip address and port
 */
err_t client_connect (client *c, const char *ipaddr, u16 port, error *e);

/**
 * Disconnects. Client must be connected
 */
void client_disconnect (client *c);

/**
 * Attempts to write as much as possible in the send buffer
 * (1 write system call)
 */
err_t client_send_some (client *c, error *e);

/**
 * Attempts to read as much as possible into the recv buffer
 * (1 read system call)
 */
err_t client_recv_some (client *c, error *e);

/**
 * Writes the entire contents of string
 * (many write system calls)
 */
err_t client_send_all (client *c, const string str, error *e);

/**
 * Writes the entire contents of string
 * (many write system calls)
 */
err_t client_recv_all (client *c, string *dest, lalloc *alloc, error *e);
