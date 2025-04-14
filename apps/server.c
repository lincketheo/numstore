#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 12345
#define BUF_SIZE 1024

int
main (void)
{
  int server_fd, client_fd;
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof (addr);
  char buf[BUF_SIZE] = { 0 };
  char *msg = "Hello from server";

  // Create socket
  if ((server_fd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  // Prepare the sockaddr_in structure
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
  addr.sin_port = htons (PORT);

  // Bind the socket to the address/port
  if (bind (server_fd, (struct sockaddr *)&addr, sizeof (addr)) < 0)
    {
      perror ("bind");
      close (server_fd);
      exit (EXIT_FAILURE);
    }

  // Listen for incoming connections
  if (listen (server_fd, 3) < 0)
    {
      perror ("listen");
      close (server_fd);
      exit (EXIT_FAILURE);
    }

  printf ("Server is listening on port %d...\n", PORT);

  // Accept an incoming connection
  if ((client_fd = accept (server_fd, (struct sockaddr *)&addr, &addr_len)) < 0)
    {
      perror ("accept");
      close (server_fd);
      exit (EXIT_FAILURE);
    }

  // Read data from the client
  int read_size = read (client_fd, buf, BUF_SIZE);
  if (read_size < 0)
    {
      perror ("read");
    }
  else
    {
      printf ("Server received: %s\n", buf);
    }

  int c2;
  if ((c2 = accept (server_fd, (struct sockaddr *)&addr, &addr_len)) < 0)
    {
      perror ("accept 2");
      close (server_fd);
      exit (EXIT_FAILURE);
    }

  // Read data from the client
  read_size = read (c2, buf, BUF_SIZE);
  if (read_size < 0)
    {
      perror ("read");
    }
  else
    {
      printf ("Server received: %s\n", buf);
    }

  // Send a greeting message back to the client
  if (send (client_fd, msg, strlen (msg), 0) < 0)
    {
      perror ("send");
    }
  else
    {
      printf ("Server sent message: %s\n", msg);
    }

  close (client_fd);
  close (c2);
  close (server_fd);
  return 0;
}
