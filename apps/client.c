#include "dev/assert.h"
#include "intf/logging.h"
#include "types.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 12346

const char *createcmd = "create a U32;";
const char *writecmd = "write 1000 a;";
const char *readcmd = "read a[0:10];";
u32 sendbuf[1000];
u32 recvbuf[1000];
int sock_fd = -1;
struct sockaddr_in serv_addr;

void
at_exit (void)
{
  i_log_info ("Done\n");
  perror ("client");
  if (sock_fd > 0)
    {
      close (sock_fd);
    }
}

int
main (void)
{
  atexit (at_exit);

  // Initialize buffers
  memset (sendbuf, 0, sizeof (sendbuf));
  memset (recvbuf, 0, sizeof (recvbuf));
  for (int i = 0; i < 1000; ++i)
    {
      sendbuf[i] = i;
    }

  // Initialize Socket
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);
  sock_fd = socket (AF_INET, SOCK_STREAM, 0);
  inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr);
  connect (sock_fd, (struct sockaddr *)&serv_addr, sizeof (serv_addr));
  i_log_info ("Client is connected to server port: %d...\n", PORT);
  perror ("client");

  // Execute transaction
  send (sock_fd, createcmd, strlen (createcmd), 0);
  i_log_info ("Sent createcmd\n");

  send (sock_fd, writecmd, strlen (writecmd), 0);
  i_log_info ("Sent writecmd\n");

  send (sock_fd, sendbuf, 1000 * sizeof *sendbuf, 0);
  i_log_info ("Sent sendbuf\n");

  send (sock_fd, readcmd, strlen (readcmd), 0);
  i_log_info ("Sent writecmd\n");

  recv (sock_fd, recvbuf, 10 * sizeof *recvbuf, 0);
  i_log_info ("Client done\n");

  // Display evidence
  for (int i = 0; i < 1000; ++i)
    {
      fprintf (stdout, "%d, ", recvbuf[i]);
    }
  return 0;
}
