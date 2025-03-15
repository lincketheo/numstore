#include <cstring>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>

int
main ()
{
  // Create a TCP socket
  int fd = socket (AF_INET, SOCK_STREAM, 0);
  if (fd == -1)
    {
      perror ("socket create");
      return -1;
    }

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons (8080);

  // Bind
  if (bind (fd, (struct sockaddr *)&address, sizeof (address)) < 0)
    {
      perror ("bind");
      return -1;
    }

  // Listen
  if (listen (fd, 3) < 0)
    {
      perror ("listen");
      return -1;
    }

  // Accept
  int addrlen = sizeof (address);
  int cfd = accept (fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
  if (cfd < 0)
    {
      perror ("accept");
      return -1;
    }

  // Read data
  char buffer[2048] = { 0 };
  while (true)
    {
      int bread = read (cfd, buffer, 2048);
      if (bread <= 0)
        break;
      printf ("%s\n", buffer);
      send (cfd, buffer, bread, 0);
      memset (buffer, 0, 2048);
    }

  close (cfd);
  close (fd);

  return 0;
}
