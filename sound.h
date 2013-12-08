#include "types.h"

#define SOUND_COM 0x04
#define SOUND_NAMBA 0x10
#define SOUND_NABMBA 0x14
#define AC_RESET 0x2c
#define READY_STATE 0x30
#define SOUND_HIDE_REG 0xf2
#define AUDIO_CODEC 0x02
#define PRIMARY_VENDOR_ID1 0x7c
#define PRIMARY_VENDOR_ID2 0x7e
#define SYBSYSTEM_VENDOR_ID 0X2c
#define SYBSYSTEM_ID 0x2e
#define PCI_CONFIG

#define DMA_BUF_NUM  32
#define DMA_SMP_NUM  0x3000
#define DMA_BUF_SIZE (DMA_SMP_NUM * 2)

#define PROCESSED  0x1
#define PCM_OUT 0x2
#define PCM_IN 0x4

struct soundNode
{
	volatile int flag;
	struct soundNode *next;
	uchar data[DMA_BUF_NUM * DMA_BUF_SIZE];
};

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

uint read_pci_config(uchar bus, uchar slot, uchar func, uchar offset);
void write_pci_config(uchar bus, uchar slot, uchar func, uchar offset, uint val);
void soundcardinit(uint addrs);
void setVolume(ushort volume);
void addSound(struct soundNode *node);
void playSound(void);
void soundInterrupt(void);
void pauseSound(void);
void setSoundSampleRate(uint samplerate);
