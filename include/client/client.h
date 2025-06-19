#pragma once

#include "errors/error.h"
#include "intf/types.h" // u16

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

/**
 * Reads into [dest] mallocs return
 * reads the first two bytes as length
 */
string client_recv (client *c, error *e);
