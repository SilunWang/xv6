#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mmu.h"
#include "proc.h"
#include "spinlock.h"
#include "buf.h"
#include "fs.h"
#include "file.h"
void play(char* file)
{

}
int main(int argc, char** argv)
{
	if(argc != 2)
	{
		printf("error format parameter");
	}
	else
	{
		play(argv[1]);
	}
	exit();

}