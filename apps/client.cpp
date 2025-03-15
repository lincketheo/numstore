#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int
main ()
{
  // Create a TCP Socket
  int fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    {
      perror ("socket create");
      return -1;
    }

  sockaddr_in saddress{};
  saddress.sin_family = AF_INET;
  saddress.sin_port = htons (8080);

  if (inet_pton (AF_INET, "127.0.0.1", &saddress.sin_addr) <= 0)
    {
      perror ("inet_pton");
      return -1;
    }

  if (connect (fd, (struct sockaddr *)&saddress, sizeof (saddress)) < 0)
    {
      perror ("connect");
      return -1;
    }

  char buffer[2048] = "Hello World";

  while (true)
    {
      send (fd, buffer, strlen (buffer), 0);
      memset (buffer, 0, 2048);
      int brecv = read (fd, buffer, 2048);
      printf ("%s\n", buffer);
    }
  return 0;
}
