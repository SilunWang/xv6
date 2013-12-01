#include "types.h"
#include "stat.h"
#include "user.h"
#include "audio.h"
#include "fcntl.h"

int
main(int argc, char *argv[])
{
  int i;
  int fd;

  struct wav info;

  fd = open(argv[1], O_RDWR);
  if (fd < 0)
  {
    printf(0, "open wav file fail\n");
    exit();
  }

  read(fd, &info, sizeof(struct wav));

  if ((info.riff_id != 0x46464952)||(info.wave_id != 0x45564157)) {
    printf(0, "invalid file format\n");
    close(fd);
    exit();
  }

  if ((info.info.id != 0x20746d66)||
      (info.info.channel != 0x0002)||
      (info.info.bytes_per_sample != 0x0004)||
      (info.info.bits_per_sample != 0x0010)) {
    printf(0, "data encoded in an unaccepted way\n");
    close(fd);
    exit();
  }

  setaudiosmprate(info.info.sample_rate);

  uint rd = 0;
  char buf[4096];
  
  while (rd < info.dlen)
  {
    read(fd, buf, (info.dlen - rd < 4096 ? info.dlen -rd : 4096));
    writeaudio(buf, (info.dlen - rd < 4096 ? info.dlen -rd : 4096));
    rd += (info.dlen - rd < 4096 ? info.dlen -rd : 4096);
  }

  memset(buf, 0, 4096);
  for (i = 0; i < DMA_BUF_NUM*DMA_BUF_SIZE/4096+1; i++)
  {
    writeaudio(buf, 4096);
  }

  close(fd);

  exit();
}
