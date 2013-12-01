#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "buf.h"
#include "audio.h"

static struct abuf audiobuf[3];
int bufcount;
int datacount;

int
sys_setaudiosmprate(void)
{
  int rate;
  //获取系统的第0个参数
  if (argint(0, &rate) < 0)
    return -1;
  
  datacount = 0;
  bufcount = 0;
  //将abuf清空并置为已处理状态
  for (int i = 0; i < 3; i++)
  {
    memset(&audiobuf[i], 0, sizeof(struct abuf));
    audiobuf[i].flags = AB_PROCESSED;
  }
  //audio.c设置采样率
  setaudiosamplerate(rate);
  return 0;
}

int
sys_setaudiovolume(void)
{
  int n;
  ushort volume;

  if (argint(0, &n) < 0)
    return -1;

  volume = ((n & 0x3F) << 8) + (n & 0x3F);

  cprintf("id");
  
  //audio.c设置音量
  setaudiovolume(volume);
  return 0;
}

int
sys_audiopause(void)
{
  audiopause();
  return 0;
}

int sys_writeaudio(void)
{
	int size;
	char *buf;
	//abuf的数据大小
	int bufsize = DMA_BUF_NUM*DMA_BUF_SIZE;
	//获取待播放的数据和数据大小
	if (argint(1, &size) < 0 || argptr(0, &buf, size) < 0)
		return -1;
	if (datacount == 0)
		memset(&audiobuf[bufcount], 0, sizeof(struct abuf));
	//若abuf的剩余大小大于数据大小，将数据写入abuf中
	if (bufsize - datacount > size)
	{
		memmove(&audiobuf[bufcount].data[datacount], buf, size);
		audiobuf[bufcount].flags = AB_PCM_OUT | AB_PROCESSED;
		datacount += size
	}
	else
	{
		int temp = bufsize - datacount;
		//abuf存满后调用audioplay进行播放
		memmove(&audiobuf[bufcount].data[datacount], buf, temp);
		audiobuf[bufcount].flags = AB_PCM_OUT;
		audioplay(&audiobuf[bufcount]);
		int flag = 0;
		//寻找一个已经被处理的abuf，将剩余数据戏写入
		while(flag)
		{
			for (int i = 0; i < 3; ++i)
			{
				if ((audiobuf[i].flags & AB_PROCESSED) == AB_PROCESSED)
				{
					memset(&audiobuf[i], 0, sizeof(struct abuf));
					if (bufsize > size - temp)
					{
						memmove(&audiobuf[i].data[0], (buf +temp), (size-temp));
						audiobuf[i].flags = AB_PCM_OUT | AB_PROCESSED;
						datacount = size - temp;
						bufcount = i;
						flag = 1;
						break;
					}
					else
					{
						memmove(&audiobuf[i].data[0], (buf +temp), bufsize);
						temp = temp + bufsize;
						audiobuf[i].flags = AB_PCM_OUT;
						audioplay(&audiobuf[i]);
					}
				}
			}
		}
	}
	return 0;
}