#include "types.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  int i;
  int fd;

  char buf[4096];

  fd = open("back", O_CREATE);
  close(fd);

  fd = open("back", O_RDWR);
  for (i = 0; i < 6475; i++)
  {
    printf(1, "\b%d", i);
    ideread(&buf, 0, 400, i*4096, 4096);
    write(fd, &buf, 4096);
  }
  ideread(&buf, 0, 400, i*512, 2092);
  write(fd, &buf, 2092);
  close(fd);
  fd = open("wav1", O_CREATE);
  close(fd);

  fd = open("wav1", O_RDWR);
  for (i = 0; i < 834; i++)
  {
    printf(1, "\b%d", i);
    ideread(&buf, 0, 60000, i*4096, 4096);
    write(fd, &buf, 4096);
  }
  ideread(&buf, 0, 60000, i*512, 3116);
  write(fd, &buf, 3116);
  close(fd);

  fd = open("wav2", O_CREATE);
  close(fd);

  fd = open("wav2", O_RDWR);
  for (i = 0; i < 1669; i++)
  {
    printf(1, "\b%d", i);
    ideread(&buf, 0, 80000, i*4096, 4096);
    write(fd, &buf, 4096);
  }
  ideread(&buf, 0, 80000, i*512, 2092);
  write(fd, &buf, 2092);
  close(fd);

  fd = open("wav3", O_CREATE);
  close(fd);

  fd = open("wav3", O_RDWR);
  for (i = 0; i < 1442; i++)
  {
    printf(1, "\b%d", i);
    ideread(&buf, 0, 100000, i*4096, 4096);
    write(fd, &buf, 4096);
  }
  ideread(&buf, 0, 100000, i*512, 1068);
  write(fd, &buf, 1068);
  close(fd);
  
  for(i = 1; i < argc; i++)
    printf(1, "%s%s waved", argv[i], i+1 < argc ? " " : "\n");
  exit();
}
