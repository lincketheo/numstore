#pragma once

#include "ds/cbuffer.h" //cbuffer
#include "intf/io.h"    // i_file
#include "intf/types.h" // u16
#include "mm/lalloc.h"  // lalloc

#include <netinet/in.h> // sockaddr_in

typedef struct client_s client;

/**
 * Creates a client and immediately connects to [ipaddr]
 */
client *client_open (const char *ipaddr, u16 port, error *e);

/**
 * Disconnects. Client must be connected
 */
void client_close (client *c);

/**
 * Writes the entire contents of string
 * (many write system calls)
 */
err_t client_send (client *c, const string str, error *e);
