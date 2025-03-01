#include "dev/assert.h"
#include "intf/logging.h"
#include "sds.h"
#include "types.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 12346
#define BUF_SIZE 1024

char createcmd[13];
char writecmd[13];
char readcmd[13];
u32 recvbuf[1000];
u32 sendbuf[1000];
int fd = -1;
int cfd = -1;
struct sockaddr_in serv_addr;
struct sockaddr_in client_addr;
socklen_t addr_len = sizeof (client_addr);

void
at_exit (void)
{
  i_log_info ("Done\n");
  perror ("server");
  if (fd >= 0)
    {
      close (fd);
    }
  if (cfd)
    {
      close (fd);
    }
}

int
main (void)
{
  atexit (at_exit);

  // Initialize socket
  fd = socket (AF_INET, SOCK_STREAM, 0);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons (PORT);
  bind (fd, (struct sockaddr *)&serv_addr, sizeof (serv_addr));
  listen (fd, 3);
  i_log_info ("Server is listening on port %d...\n", PORT);

  // Listen for transaction
  cfd = accept (fd, (struct sockaddr *)&client_addr, &addr_len);
  i_log_info ("Server got connection\n");
  recv (cfd, createcmd, 13, 0);
  i_log_info ("Got createcmd: %s\n", createcmd);

  recv (cfd, writecmd, 13, 0);
  i_log_info ("Got writecmd: %s\n", writecmd);

  recv (cfd, recvbuf, 1000 * sizeof *recvbuf, 0);
  i_log_info ("Got recvbuf\n");

  recv (cfd, readcmd, 13, 0);
  i_log_info ("Got readcmd: %s\n", readcmd);

  send (cfd, recvbuf, 10 * sizeof *recvbuf, 0);
  i_log_info ("Server done\n");

  return 0;
}
