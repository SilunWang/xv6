#include "types.h"
#include "defs.h"
#include "x86.h"

#define PCI_CONFIG_COMMAND 0xcf8
#define PCI_CONFIG_DATA 0xcfc

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
