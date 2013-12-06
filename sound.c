#include "sound.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "traps.h"
#include "spinlock.h"
#include "proc.h"

#define PCI_CONFIG_COMMAND 0xcf8
#define PCI_CONFIG_DATA 0xcfc

//Native Audio Mixer Base Address
ushort AUDIO_IO_SPACE_NAMBA;
#define NAMBA_PCMV AUDIO_IO_SPACE_NAMBA + 0x20 //volume? General Purpose
#define FRONT_DAC_RATE AUDIO_IO_SPACE_NAMBA + 0x2C
#define SURROUND_DAC_RATE AUDIO_IO_SPACE_NAMBA + 0x2E
#define LFE_DAC_RATE AUDIO_IO_SPACE_NAMBA + 0x30

//Native Audio Bus Mastering Base Address
ushort AUDIO_IO_SPACE_NABMBA;
#define PO_BDBAR AUDIO_IO_SPACE_NABMBA + 0x10//PCM Out Buffer Descriptor list Base Address Register 
#define PO_LVI AUDIO_IO_SPACE_NABMBA + 0x15//PCM Out Last Valid Index 
#define PO_SR AUDIO_IO_SPACE_NABMBA + 0x16//PCM Out Status Register
#define PO_CR AUDIO_IO_SPACE_NABMBA + 0x1B //PCM Out Control Register
#define MC_BDBAR AUDIO_IO_SPACE_NABMBA + 0x20//Mic. In Buffer Descriptor list Base Address Register
#define MC_LVI AUDIO_IO_SPACE_NABMBA + 0x25//Mic. In Last Valid Index
#define MC_SR AUDIO_IO_SPACE_NABMBA + 0x26//Mic. In Status Register
#define MC_CR AUDIO_IO_SPACE_NABMBA + 0x2B//Mic. In Control Register

ushort SOUND_NAMBA_DATA;
ushort SOUND_NABMBA_DATA;
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
                soundcardinit(bus, slot, func);
                return;
            }
        }
    }
	cprintf("Sound card not found!\n");
}

void soundcardinit(uchar bus, uchar slot, uchar func)
{
	uint tmp, reflection, cur, vendorID;
	ushort vendorID1, vendorID2;
	
	//Initializing the Audio I/O Space
	tmp = read_pci_config(bus, slot, func, SOUND_COM & 0xfc);
	write_pci_config(bus, slot, func, SOUND_COM & 0xfc, tmp | 0x05);
	SOUND_NAMBA_DATA = read_pci_config(bus, slot, func, SOUND_NAMBA & 0xfc) & (~0x1);
	SOUND_NABMBA_DATA = read_pci_config(bus, slot, func, SOUND_NABMBA & 0xfc) & (~0x1);
	cprintf("AUDIO I/O Space initialized successfully!\n");
	
	//Removing AC_RESET
	outb(SOUND_NABMBA_DATA + AC_RESET, 0x2);
	cprintf("AC_RESET removed successfully!\n");
	
	//Reading Codec Ready Status
	cur = 0;
	cprintf("Waiting for Codec Ready Status...\n");
	while (1)
	{
		++cur;
		//wait and read
		reflection = inw(SOUND_NABMBA_DATA + READY_STATE);
		if ((reflection & 0x100) != 0)
        break;
		//timeout
		if (cur > 1000000)
		{
			cprintf("Failed for reading codec ready status!\n");
			return;
		}
	}
	cprintf("Codec is ready!\n");
	
	//Determine Audio Codec
	tmp = inw(SOUND_NAMBA_DATA + AUDIO_CODEC);
	outw(SOUND_NAMBA_DATA + AUDIO_CODEC, 0x8000);
	if (inw(SOUND_NAMBA_DATA + AUDIO_CODEC) != 0x8000)
	{
		cprintf("Audio Codec Function not found!\n");
		return;
	}
	tmp = inw(SOUND_NAMBA_DATA + AUDIO_CODEC);
	cprintf("Audio Codec Function is found, current volume is %x.\n", tmp);
	
	//Reading the Audio Codec Vendor ID
	vendorID1 = inw(SOUND_NAMBA_DATA + PRIMARY_VENDOR_ID1);
	vendorID2 = inw(SOUND_NAMBA_DATA + PRIMARY_VENDOR_ID2);
	cprintf("Audio Codec Vendor ID read successfully!\n");
	
	//Programming the PCI Audio Subsystem ID
	vendorID = (vendorID2 << 16) + vendorID1;
	write_pci_config(bus, slot, func, SYBSYSTEM_VENDOR_ID, vendorID);
	
	//Initailize Interruption
	initlock(&soundLock, "audio");
	picenable(IRQ_SOUND);
	ioapicenable(IRQ_SOUND, ncpu - 1);	
	
	outw(SOUND_NAMBA + AUDIO_CODEC, 0x1000);

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

