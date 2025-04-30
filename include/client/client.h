#pragma once

#include "dev/errors.h"
#include "ds/cbuffer.h"
#include "intf/io.h"
#include "intf/types.h"

#include <netinet/in.h>

typedef struct
{
  i_file sfd;
  struct sockaddr_in server_addr;

  cbuffer send;
  cbuffer recv;

  u8 _send[10];
  u8 _recv[10];
} client;

void client_create (client *dest);

err_t client_connect (client *c, const char *ipaddr, u16 port);
void client_disconnect (client *c);

err_t client_send_some (client *c);
err_t client_recv_some (client *c);

err_t client_execute_all (client *c, const string str);
