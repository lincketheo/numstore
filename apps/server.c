#include "server.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12346
#define BUF_SIZE 1024

int
main (void)
{
  server s;
  connection c;

  if (server_setup (&s))
    {
      return -1;
    }

  if (server_execute (&c, &s))
    {
      return -1;
    }

  if (connection_write_execute (&c))
    {
      return -1;
    }

  if (connection_read_execute (&c))
    {
      return -1;
    }

  close (s.fd);

  return 0;
}
