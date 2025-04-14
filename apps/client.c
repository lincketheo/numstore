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
  int sock_fd;
  struct sockaddr_in serv_addr;
  char buf[BUF_SIZE] = { 0 };
  char *msg = "Hello from client";

  // Create socket
  if ((sock_fd = socket (AF_INET, SOCK_STREAM, 0)) < 0)
    {
      perror ("socket");
      exit (EXIT_FAILURE);
    }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons (PORT);

  // Convert IPv4 address from text to binary form (connect to localhost)
  if (inet_pton (AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0)
    {
      perror ("inet_pton");
      close (sock_fd);
      exit (EXIT_FAILURE);
    }

  // Connect to the server
  if (connect (sock_fd, (struct sockaddr *)&serv_addr, sizeof (serv_addr)) < 0)
    {
      perror ("connect");
      close (sock_fd);
      exit (EXIT_FAILURE);
    }

  // Send a message to the server
  if (send (sock_fd, msg, strlen (msg), 0) < 0)
    {
      perror ("send");
    }
  else
    {
      printf ("Client sent message: %s\n", msg);
    }

  // Receive a reply from the server
  int read_size = read (sock_fd, buf, BUF_SIZE);
  if (read_size < 0)
    {
      perror ("read");
    }
  else
    {
      printf ("Client received: %s\n", buf);
    }

  close (sock_fd);
  return 0;
}
