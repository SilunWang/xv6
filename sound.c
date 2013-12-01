#include "types.h"
#include "defs.h"
#include "x86.h"
#include "spinlock.h"
#include "sound.h"

#define PCI_CONFIG_COMMAND 0xcf8
#define PCI_CONFIG_DATA 0xcfc

//Native Audio Mixer Base Address
ushort AUDIO_IO_SPACE_NAMBA;
#define GENERAL_PURPO 0x20 //General Purpose

//Native Audio Bus Mastering Base Address
ushort AUDIO_IO_SPACE_NABMBA;
#define PO_BDBAR 0x10//Buffer基地址 PCM Out Buffer Descriptor list Base Address Register 
#define PO_CIV 0x14//没用上 PCM Out Current Index Value
#define PO_LVI 0x15//PCM Out Last Valid Index 
#define PO_SR 0x16//PCM Out Status Register
#define PO_CR 0x1B //PCM Out Control Register
#define MC_BDBAR 0x20//Mic. In Buffer Descriptor list Base Address Register 
#define PM_CIV 0x24//Mic. In Current Index Value 
#define MC_LVI 0x25//Mic. In Last Valid Index
#define MC_SR 0x26//Mic. In Status Register
#define MC_CR 0x2B//Mic. In Control Register

static struct spinlock soundLock;
static struct soundNode *soundQueue;

struct descriptor{
  uint buf;
  uint cmd_len;
};

static struct descriptor desTable[DMA_BUF_NUM];

uint read_pci_config(uchar bus, uchar slot, uchar func, uchar offset)
{
	uint tmp, res;
	tmp = 0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | offset;
	outsl(0xcf8, &tmp, 1);
	insl(0xcfc, &res, 1);
	return res;
}

void write_pci_config(uchar bus, uchar slot, uchar func, uchar offset, uint val)
{
	uint tmp;
	tmp = 0x80000000 | (bus << 16) | (slot << 11) | (func << 8) | offset;
	outsl(0xcf8, &tmp, 1);
	outsl(0xcfc, &val, 1);
}

void soundinit(void)
{
	uchar bus, slot, func;
	ushort vendor, device;
	uint res;
	for (bus = 0; bus < 5; ++bus)
		for (slot = 0; slot < 32; ++slot)
			for (func = 0; func < 8; ++func)
			{
				res = read_pci_config(bus, slot, func, 0);
				if (res != 0xFFFFFFFF)
				{
					vendor = res & 0xFFFF;
					device = (res >> 16) & 0xFFFF;
					if (vendor == 0x8086 && device == 0x2415)
					{
						cprintf("Find sound card!\n");
						return;
					}
				}
			}
}

void addSound(struct soundNode *node)
{
  struct soundNode *ptr;

  acquire(&soundLock);

  node->next = NULL;
  ptr = soundQueue;

  while(ptr)
  {
  	ptr = ptr->next;
  }
  ptr = node;

  if(soundQueue == node)
  {
    playSound();
  }

  release(&soundLock);
}

void playSound()
{
	int i;

	//遍历声卡DMA的描述符列表，初始化每一个描述符buf指向缓冲队列中第一个音乐的数据块
	//每个数据块大小: DMA_BUF_SIZE
	for (i = 0; i < DMA_BUF_NUM; i++)
	{
    	desTable[i].buf = (uint)(soundQueue->data) + i * DMA_BUF_SIZE;
    	desTable[i].cmd_len = 0x80000000 + DMA_SMP_NUM;
	}

	uint base = (uint)desTable;

	//开始播放
	if ((soundQueue->flag & PCM_OUT) == PCM_OUT)
	{
    	//init base register
    	outsl(AUDIO_IO_SPACE_NABMBA + PO_BDBA, &base, 1);
    	//init last valid index
    	outb(AUDIO_IO_SPACE_NABMBA + MC_LVI, 0x1F);
    	//init control register
    	outb(AUDIO_IO_SPACE_NABMBA + MC_CR, 0x05);
	}

	//开始录音
	else if ((soundQueue->flag & PCM_IN) == PCM_IN)
	{
    	//init register
    	outsl(AUDIO_IO_SPACE_NABMBA + PO_BDBA, &base, 1);
    	outb(AUDIO_IO_SPACE_NABMBA + MC_LVI, 0x1F);
    	outb(AUDIO_IO_SPACE_NABMBA + MC_CR, 0x05);
	}

}

void soundInterrupt(void)
{
  int i;

  acquire(&soundLock);

  struct soundNode *node = soundQueue;
  soundQueue = node->next;

  //flag
  int flag = node->flag;

  node->flag |= PROCESSED;

  //0 sound file left
  if (soundQueue == 0)
  {
    if ((flag & PCM_OUT) == PCM_OUT)
    {
      ushort sr = inw(AUDIO_IO_SPACE_NABMBA + PO_SR);
      outw(AUDIO_IO_SPACE_NABMBA + PO_SR, sr);
    }
    else if ((flag & AB_PCM_IN) == PCM_IN)
    {
      ushort sr = inw(AUDIO_IO_SPACE_NABMBA + MC_SR);
      outw(AUDIO_IO_SPACE_NABMBA + MC_SR, sr);
    }
    cprintf("Play Done\n");
    release(&soundLock);
    return;
  }

  //descriptor table buffer
  for (i = 0; i < DMA_BUF_NUM; i++)
  {
    desTable[i].buf = (uint)(soundQueue->data) + i * DMA_BUF_SIZE;
    desTable[i].cmd_len = 0x80000000 + DMA_SMP_NUM;
  }

  //play music
  if ((flag & PCM_OUT) == PCM_OUT)
  {
    ushort sr = inw(AUDIO_IO_SPACE_NABMBA + PO_SR);
    outw(AUDIO_IO_SPACE_NABMBA + PO_SR, sr);
    outb(AUDIO_IO_SPACE_NABMBA + PO_CR, 0x05);
  }

  //record
  else if ((flag & PCM_IN) == PCM_IN)
  {
    ushort sr = inw(AUDIO_IO_SPACE_NABMBA + MC_SR);
    outw(AUDIO_IO_SPACE_NABMBA + MC_SR, sr);
    outb(AUDIO_IO_SPACE_NABMBA + MC_CR, 0x05);
  }
  
  release(&soundLock);
}
