#include "types.h"
#include "x86.h"
#include "defs.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"
#include "buf.h"
#include "audio.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

int
sys_ideread(void)
{
  char* dst;
  int dev, sector, offset, length;
  int i;

  if (argint(1, &dev) < 0 || argint(2, &sector) < 0 || argint(3, &offset) < 0 || argint(4, &length) < 0 || argptr(0, &dst, length) < 0)
    return -1;

  struct buf* temp;
	uchar* tempfordst;
	uint WhereToBegin = 0;
	if (offset >= 512)
	{
		WhereToBegin = offset / 512;
		offset = offset % 512;
	}	
  uint nsector = (length + offset) / 512;
  uint lastbyte = (length + offset) % 512;
  temp = bread(dev,sector + WhereToBegin);
  memmove(dst, &(temp->data) + offset, (512 - offset < length ? 512 - offset : length));
  tempfordst = (uchar *)((uint)dst + 512 - offset);
  brelse(temp);
  for (i = 1; i < nsector; i++)
  {
	  temp = bread(dev, sector + i + WhereToBegin);
	  memmove(tempfordst, &(temp->data), 512);
	  tempfordst += 512;
	  brelse(temp);
  }
  if (lastbyte != length + offset)
  {
    if (lastbyte != 0)
    {
	    temp = bread(dev, sector + i + WhereToBegin);
	    memmove(tempfordst, &(temp->data), lastbyte);
	    brelse(temp);
	  }
	}
	return 0;
}

// return how many clock tick interrupts have occurred
// since boot.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
