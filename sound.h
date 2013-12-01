#define DMA_BUF_NUM  32
#define DMA_SMP_NUM  0x8000
#define DMA_BUF_SIZE (DMA_SMP_NUM * 2)

#define PROCESSED  0x1
#define PCM_OUT 0x2
#define PCM_IN 0x4

struct soundNode
{
	volatile int flag;
	struct soundNode *next;
	uchar data[DMA_BUF_NUM * DMA_BUF_SIZE];
}