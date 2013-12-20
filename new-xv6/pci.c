#include "types.h"
#include "defs.h"
#include "x86.h"

#define PCI_CONFIG_COMMAND       0xCF8
#define PCI_CONFIG_DATA       0xCFC

void 
pciwrite(ushort addr, uchar reg, uint data)
{
  uint temp;

  temp = addr;
  temp <<= 8;
  temp |= 0x80000000;
  temp |= (reg & 0xFC);

  outsl(PCI_CONFIG_COMMAND, &temp, 1);
  outsl(PCI_CONFIG_DATA, &data, 1);
}

uint 
pciread(ushort addr, uchar reg)
{
  uint temp;

  temp = addr;
  temp <<= 8;
  temp |= 0x80000000;
  temp |= (reg & 0xFC);

  outsl(PCI_CONFIG_COMMAND, &temp, 1);
  
  uint result;
  insl(PCI_CONFIG_DATA, &result, 1);
  
  return result;
}

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
                soundcardinit((bus << 8) + (slot << 3) + func);
                return;
            }
        }
      }
  cprintf("Sound card not found!\n");
}

