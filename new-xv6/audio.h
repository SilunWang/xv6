#define DMA_BUF_NUM  32
#define DMA_SMP_NUM  0x80
#define DMA_BUF_SIZE (DMA_SMP_NUM*2)

struct fmt {
  uint id;
  uint len;
  ushort pad;
  ushort channel;
  uint sample_rate;
  uint bytes_per_sec;
  ushort bytes_per_sample;
  ushort bits_per_sample;
};

struct wav{
  uint riff_id;
  uint rlen;
  uint wave_id;
  struct fmt info;
  uint data_id;
  uint dlen;
};

struct soundNode{
  volatile int flag;
  struct soundNode *next;
  uchar data[DMA_BUF_NUM*DMA_BUF_SIZE];
};

void pauseSound(void);
void setVolume(ushort volume);
void addSound(struct soundNode *node);

#define PROCESSED  0x1
#define PCM_OUT 0x2
#define PCM_IN 0x4
