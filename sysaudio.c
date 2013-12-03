#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "buf.h"
#include "sound.h"

struct soundNode audiobuf[3];
int bufcount;
int datacount;

int
sys_setaudiosmprate(void)
{
  int rate, i;
  //获取系统的第0个参数
  if (argint(0, &rate) < 0)
    return -1;
  
  datacount = 0;
  bufcount = 0;
  //将soundNode清空并置为已处理状态
  for (i = 0; i < 3; i++)
  {
    memset(&audiobuf[i], 0, sizeof(struct soundNode));
    audiobuf[i].flag = PROCESSED;
  }
  //audio.c设置采样率
  setSoundSampleRate(rate);
    return 0;
}

int
sys_setaudiovolume(void)
{
/*  int n;
//  ushort volume;

  if (argint(0, &n) < 0)
    return -1;

//  volume = ((n & 0x3F) << 8) + (n & 0x3F);

  cprintf("id");
  
  //audio.c设置音量
//  setaudiovolume(volume);
*/  return 0;
}

int
sys_audiopause(void)
{
 // pauseSound();
  return 0;
}

int sys_writeaudio(void)
{
/*	int size;
	char *buf;
	//soundNode的数据大小
	int bufsize = DMA_BUF_NUM*DMA_BUF_SIZE;
	//获取待播放的数据和数据大小
	if (argint(1, &size) < 0 || argptr(0, &buf, size) < 0)
		return -1;
	if (datacount == 0)
		memset(&audiobuf[bufcount], 0, sizeof(struct soundNode));
	//若soundNode的剩余大小大于数据大小，将数据写入soundNode中
	if (bufsize - datacount > size)
	{
		memmove(&audiobuf[bufcount].data[datacount], buf, size);
		audiobuf[bufcount].flag = PCM_OUT | PROCESSED;
		datacount += size;
	}
	else
	{
		int temp = bufsize - datacount, i;
		//soundNode存满后调用audioplay进行播放
		memmove(&audiobuf[bufcount].data[datacount], buf, temp);
		audiobuf[bufcount].flag = PCM_OUT;
		addSound(&audiobuf[bufcount]);
		int flag = 0;
		//寻找一个已经被处理的soundNode，将剩余数据戏写入
		while(flag)
		{
			for (i = 0; i < 3; ++i)
			{
				if ((audiobuf[i].flag & PROCESSED) == PROCESSED)
				{
					memset(&audiobuf[i], 0, sizeof(struct soundNode));
					if (bufsize > size - temp)
					{
						memmove(&audiobuf[i].data[0], (buf +temp), (size-temp));
						audiobuf[i].flag = PCM_OUT | PROCESSED;
						datacount = size - temp;
						bufcount = i;
						flag = 1;
						break;
					}
					else
					{
						memmove(&audiobuf[i].data[0], (buf +temp), bufsize);
						temp = temp + bufsize;
						audiobuf[i].flag = PCM_OUT;
						addSound(&audiobuf[i]);
					}
				}
			}
		}
	}
*/	return 0;
}