#include "sound.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "traps.h"
#include "spinlock.h"
#include "proc.h"

#define PCI_CONFIG_SPACE_STA_CMD 0x4
#define PCI_CONFIG_SPACE_NAMBA 0X10
#define PCI_CONFIG_SPACE_NABMBA 0x14
#define PCI_CONFIG_SPACE_SID_SVID 0x2c
#define PCI_CONFIG_SPACE_INTRL 0x3c

#define PCI_CONFIG_COMMAND 0xcf8
#define PCI_CONFIG_DATA 0xcfc

#define NABMBA_GLOB_CNT 0x2c
#define NABMBA_GLOB_STA 0x30
#define NAMBA_PCVID1 0x7c
#define NAMBA_PCVID2 0x7e
//Native Audio Mixer Base Address
ushort SOUND_NAMBA_DATA;
#define NAMBA_PCMV SOUND_NAMBA_DATA + 0x2 //volume? General Purpose
#define FRONT_DAC_RATE SOUND_NAMBA_DATA + 0x2C
#define SURROUND_DAC_RATE SOUND_NAMBA_DATA + 0x2E
#define LFE_DAC_RATE SOUND_NAMBA_DATA + 0x30

//Native Audio Bus Mastering Base Address
ushort SOUND_NABMBA_DATA;
#define PO_BDBAR SOUND_NABMBA_DATA + 0x10//PCM Out Buffer Descriptor list Base Address Register 
#define PO_LVI SOUND_NABMBA_DATA + 0x15//PCM Out Last Valid Index 
#define PO_SR SOUND_NABMBA_DATA + 0x16//PCM Out Status Register
#define PO_CR SOUND_NABMBA_DATA + 0x1B //PCM Out Control Register
#define MC_BDBAR SOUND_NABMBA_DATA + 0x20//Mic. In Buffer Descriptor list Base Address Register
#define MC_LVI SOUND_NABMBA_DATA + 0x25//Mic. In Last Valid Index
#define MC_SR SOUND_NABMBA_DATA + 0x26//Mic. In Status Register
#define MC_CR SOUND_NABMBA_DATA + 0x2B//Mic. In Control Register

static struct spinlock soundLock;
static struct soundNode *soundQueue;

struct descriptor
{
    uint buf;
    uint cmd_len;
};

static struct descriptor descriTable[DMA_BUF_NUM];

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

void pciwrite(ushort addr, uchar reg, uint data)
{
	uint tmp;
	tmp = addr;
	tmp <<= 8;
	tmp |= 0x80000000;
	tmp |= (reg & 0xfc);
	outsl(PCI_CONFIG_COMMAND, &tmp, 1);
	outsl(PCI_CONFIG_DATA, &data, 1);
}

uint pciread(ushort addr, uchar reg)
{
	uint tmp;
	tmp = addr;
	tmp <<= 8;
	tmp |= 0x80000000;
	tmp |= (reg & 0xfc);
	outsl(PCI_CONFIG_COMMAND, &tmp, 1);
	uint result;
	insl(PCI_CONFIG_DATA, &result, 1);
	return result;
}

void soundinit(void)
{
	uchar bus, slot, func;
	ushort vendor, device;
	uint res;
	// search bus, slot and func to find Intel 82801 AA AC'97 sound card
	for (bus = 0; bus < 5; ++bus)
    for (slot = 0; slot < 32; ++slot)
    for (func = 0; func < 8; ++func)
    {
        res = read_pci_config(bus, slot, func, 0);
        if (res != 0xffffffff)
        {
            vendor = res & 0xffff;
            device = (res >> 16) & 0xffff;
            // find 0x24158086(Intel 82801
            if (vendor == 0x8086 && device == 0x2415)
            {
                cprintf("Find sound card!\n");
                // Init sound
                soundcardinit((bus << 8) + (slot << 3) + func);
                return;
            }
        }
    }
	cprintf("Sound card not found!\n");
}

void soundcardinit(uint addr)
{
	uint tmp, reflection, cur, vendorID;
	ushort vendorID1, vendorID2;
	
	//Initializing the Audio I/O Space
	tmp = pciread(addr, PCI_CONFIG_SPACE_STA_CMD);
	pciwrite(addr, PCI_CONFIG_SPACE_STA_CMD, tmp | 0x5);

	SOUND_NAMBA_DATA = pciread(addr, PCI_CONFIG_SPACE_NAMBA) & (~0x1);
	SOUND_NABMBA_DATA = pciread(addr, PCI_CONFIG_SPACE_NABMBA) & (~0x1);
	cprintf("AUDIO I/O Space initialized successfully!\n");
	
	//Removing AC_RESET
	outb(SOUND_NABMBA_DATA + NABMBA_GLOB_CNT, 0x2);
	cprintf("AC_RESET removed successfully!\n");
	
	//Reading Codec Ready Status
	cur = 0;
	cprintf("Waiting for Codec Ready Status...\n");
	while (!(inw(SOUND_NABMBA_DATA + NABMBA_GLOB_STA) & 0X100) && cur < 1000)
	{
		cur++;
	}
	if (cur == 1000)
	{
		cprintf("\nAudio Init Failed\n");
		return;
	}
	cprintf("Codec is ready!\n");
	
	//Determine Audio Codec
	tmp = inw(NAMBA_PCMV);
	cprintf("%x\n", tmp);
	outw(NAMBA_PCMV, 0x8000);
	if (inw(NAMBA_PCMV) != 0x8000)
	{
		cprintf("Audio Codec Function not found!\n");
		return;
	}
	outw(NAMBA_PCMV, tmp);
	cprintf("Audio Codec Function is found, current volume is %x.\n", tmp);
	
	//Reading the Audio Codec Vendor ID
	vendorID1 = inw(SOUND_NAMBA_DATA + NAMBA_PCVID1);
	vendorID2 = inw(SOUND_NAMBA_DATA + NAMBA_PCVID2);
	cprintf("Audio Codec Vendor ID read successfully!\n");
	
	//Programming the PCI Audio Subsystem ID
	vendorID = (vendorID2 << 16) + vendorID1;
	pciwrite(addr, PCI_CONFIG_SPACE_SID_SVID, vendorID);
	
	//Initailize Interruption
	initlock(&soundLock, "audio");
	picenable(IRQ_SOUND);
	ioapicenable(IRQ_SOUND, ncpu - 1);	
	
	outw(NAMBA_PCMV, 0x1f1f);

    soundQueue = 0;
}

void setVolume(ushort volume)
{
  //NAMBA_PCMV --> volume
  cprintf("setVolume\n");
  outw(NAMBA_PCMV, volume);
}

//add sound-piece to the end of queue
void addSound(struct soundNode *node)
{   
    cprintf("addSound\n");
    struct soundNode **ptr;

    acquire(&soundLock);

    node->next = 0;
    for(ptr = &soundQueue; *ptr; ptr = &(*ptr)->next)
        ;
    *ptr = node;


    //node is already the first
    //play sound
    if (soundQueue == node)
    {
        playSound();
    }

    release(&soundLock);
}

void playSound(void)
{
    cprintf("playSound\n");
    int i;

    //遍历声卡DMA的描述符列表，初始化每一个描述符buf指向缓冲队列中第一个音乐的数据块
    //每个数据块大小: DMA_BUF_SIZE
    for (i = 0; i < DMA_BUF_NUM; i++)
    {
        descriTable[i].buf = (uint)(soundQueue->data) + i * DMA_BUF_SIZE;
        descriTable[i].cmd_len = 0x80000000 + DMA_SMP_NUM;
    }

    uint base = (uint)descriTable;

    //开始播放: PCM_OUT
    if ((soundQueue->flag & PCM_OUT) == PCM_OUT)
    {
        cprintf("PCM_OUT\n");
        //init base register
        //将内存地址base开始的1个双字写到PO_BDBAR
        outsl(PO_BDBAR, &base, 1);
        //init last valid index
        outb(PO_LVI, 0x1F);
        //init control register
        //run audio
        //enable interrupt
        outb(PO_CR, 0x05);
    }

    //开始录音: Mic_IN
    else if ((soundQueue->flag & PCM_IN) == PCM_IN)
    {
        cprintf("PCM_IN\n");
        //init register
        //将内存地址base开始的1个双字写到PO_BDBAR
        outsl(MC_BDBAR, &base, 1);
        //init last valid index
        outb(MC_LVI, 0x1F);
        //init control register
        //run audio
        //enable interrupt
        outb(MC_CR, 0x05);
    }

}

void soundInterrupt(void)
{
    cprintf("soundInter");
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
            //????
            ushort sr = inw(PO_SR);
            outw(PO_SR, sr);
        }
        else if ((flag & PCM_IN) == PCM_IN)
        {
            ushort sr = inw(MC_SR);
            outw(MC_SR, sr);
        }
        cprintf("Play Done\n");
        release(&soundLock);
        return;
    }

    //descriptor table buffer
    for (i = 0; i < DMA_BUF_NUM; i++)
    {
        descriTable[i].buf = (uint)(soundQueue->data) + i * DMA_BUF_SIZE;
        descriTable[i].cmd_len = 0x80000000 + DMA_SMP_NUM;
    }

    //play music
    if ((flag & PCM_OUT) == PCM_OUT)
    {
        ushort sr = inw(PO_SR);
        outw(PO_SR, sr);
        outb(PO_CR, 0x05);
    }

    //record
    else if ((flag & PCM_IN) == PCM_IN)
    {
        ushort sr = inw(MC_SR);
        outw(MC_SR, sr);
        outb(MC_CR, 0x05);
    }

    release(&soundLock);
}

void pauseSound(void)
{
    cprintf("pauseSound\n");
    // get Control Register
    uchar temp = inb(PO_CR);

    if (temp != 0x00)
        outb(PO_CR, 0x00);
    else
        outb(PO_CR, 0x05);
}

void setSoundSampleRate(uint samplerate)
{
    cprintf("setSoundSampleRate");
    //Control Register --> 0x00
    //pause audio
    //disable interrupt
    outb(PO_CR, 0x00);

    //PCM Front DAC Rate
    outw(FRONT_DAC_RATE, samplerate & 0xFFFF);
    //PCM Surround DAC Rate
    outw(SURROUND_DAC_RATE, samplerate & 0xFFFF);
    //PCM LFE DAC Rate
    outw(LFE_DAC_RATE, samplerate & 0xFFFF);
}

