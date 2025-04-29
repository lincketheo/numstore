#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int
main ()
{
  int sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      exit (1);
    }

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons (12345);
  if (inet_pton (AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0)
    {
      perror ("inet_pton");
      exit (1);
    }

  if (connect (sock, (struct sockaddr *)&server_addr, sizeof (server_addr)) < 0)
    {
      perror ("connect");
      exit (1);
    }

  const char *msg = "Hello World\n";
  ssize_t sent = send (sock, msg, strlen (msg), 0);
  if (sent < 0)
    {
      perror ("send");
      exit (1);
    }

  printf ("Sent %ld bytes\n", sent);

  // Now read a response
  char buf[256];
  ssize_t recvd = recv (sock, buf, sizeof (buf) - 1, 0);
  if (recvd < 0)
    {
      perror ("recv");
      exit (1);
    }
  buf[recvd] = '\0'; // Null-terminate the received data

  printf ("Received %ld bytes: %s\n", recvd, buf);

  close (sock);

  return 0;
}
