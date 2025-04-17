#include "os/types.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12347

int
main (void)
{
  int sock_fd;
  struct sockaddr_in serv_addr;

  u32 buf[2048] = { 0 };
  u32 buf2[2048] = { 0 };
  for (int i = 0; i < 2048; ++i)
    {
      buf[i] = i;
    }

  if ((sock_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);

  if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      perror ("inet_pton");
      close (sock_fd);
      exit (EXIT_FAILURE);
    }

  if (connect (sock_fd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
    {
      perror ("connect");
      close (sock_fd);
      exit (EXIT_FAILURE);
    }

  if (send (sock_fd, buf, 2048 * sizeof (int), 0) < 0)
    {
      perror ("send");
    }

  if (read (sock_fd, buf2, 2048 * sizeof (int)) < 0)
    {
      perror ("read");
    }

  for (int i = 0; i < 2048; ++i)
    {
      fprintf (stdout, "%d ", buf2[i]);
    }
  fprintf (stdout, "\n");

  close (sock_fd);

  return 0;
}
