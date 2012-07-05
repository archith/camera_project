/*
 *  Copyright (C) 2006-2008 Prolific Technology Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/config.h>

#ifdef CONFIG_PCI

#include <linux/types.h>
#include <linux/pci.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>

#if defined(CONFIG_ARCH_PL1071)  || defined(CONFIG_ARCH_PL1091)
#else
#include <asm/addrspace.h>
#endif
#include <asm/pl_reg.h>
#include <asm/arch/irqs.h>

#undef PCI_SCAN_DEBUG
/* define SHARE_MEMORY_RESOURCE for slot1~slot4 memory resource sharing */
#define SHARE_MEMORY_RESOURCE      

#define PCI_ACCESS_READ  0
#define PCI_ACCESS_WRITE 1

#define PCI0_CFGADDR_BUSNUM_SHF     16
#define PCI0_CFGADDR_FUNCTNUM_SHF   8
#define PCI0_CFGADDR_REGNUM_SHF     2
#define PCI0_CFGADDR_CONFIGEN_BIT   (1 << 31)


/* for PL PCI Host Bridge */
#define PROLIFIC_PCI_ID				0x180D
#define PROLIFIC_PCI_HOSTBRIDGE_ID		0x1A00
#define PROLIFIC_PCI_HOSTBRIDGE_REVISION	1

/* for PL USB Open Host Controller */
#define PL_PCI_USB_DEVICE		4+1
#define PROLIFIC_PCI_USBOHCI_ID		0x2300
#define PROLIFIC_PCI_USBOHCI_REVISION	1

#define PCI_CLASS_UHCI_INF		0x00
#define PCI_CLASS_OHCI_INF		0x10
#define PCI_CLASS_USB2_INF		0x20

#define PL_USB_BASE_ADDRESS		SOC_IO_SPACE_BASE
#define PL_USB_BAR_SIZING		(~0xFFF)

#if defined (CONFIG_ARCH_PL1071) || defined(CONFIG_ARCH_PL1091)
/*
 * Convert from Linux-centric to bus-centric addresses for bridge devices.
 */
void __init
pcibios_fixup_pbus_ranges(struct pci_bus *bus, struct pbus_set_ranges_data *ranges)
{
/*
	struct pci_sys_data *root = bus->sysdata;

	ranges->mem_start -= root->mem_offset;
	ranges->mem_end -= root->mem_offset;
	ranges->prefetch_start -= root->mem_offset;
	ranges->prefetch_end -= root->mem_offset;
*/  
}
void __init pcibios_update_irq(struct pci_dev *dev, int irq)
{
	pci_write_config_byte(dev, PCI_INTERRUPT_LINE, irq);
}

#endif



#ifdef CONFIG_USB
static unsigned int
pl_pcibios_usb_access(unsigned char access_type, struct pci_dev *dev,
                           unsigned char where, unsigned int data)
{
	static int bar_sizing = 0;
	if (access_type == PCI_ACCESS_WRITE) {
		if (where == PCI_BASE_ADDRESS_0 && data == ~0) 
			bar_sizing = 1;
		return data;
	}

    switch(where) {
    case PCI_VENDOR_ID:
    case PCI_DEVICE_ID:
        return (PROLIFIC_PCI_USBOHCI_ID << 16) | PROLIFIC_PCI_ID;
	case PCI_COMMAND:
    case PCI_STATUS:
		return (PCI_COMMAND_MASTER | PCI_COMMAND_MEMORY) | 
        	   (PCI_STATUS_DEVSEL_MEDIUM | PCI_STATUS_CAP_LIST) << 16 ;
    case PCI_CLASS_REVISION:
    case PCI_CLASS_DEVICE:
        return  (PCI_CLASS_SERIAL_USB << 16) | 
				(PCI_CLASS_OHCI_INF << 8) | 
				PROLIFIC_PCI_USBOHCI_REVISION;

	case PCI_CACHE_LINE_SIZE:
    case PCI_HEADER_TYPE:
	case PCI_LATENCY_TIMER:
		return 0x08 | 32 << 8 | PCI_HEADER_TYPE_NORMAL << 16 | 0 << 24;
	case PCI_BASE_ADDRESS_0:
		if (bar_sizing) {
			bar_sizing = 0;
			return PL_USB_BAR_SIZING;
		}
		return PL_USB_BASE_ADDRESS;
	case PCI_INTERRUPT_LINE:
	case PCI_INTERRUPT_PIN:
	case PCI_MIN_GNT:
	case PCI_MAX_LAT:
		return IRQ_PL_USB | (1 << 8) | (1 << 16) | (42 << 24);

    default:
        return 0;
    }
	
	return 0;
}

#endif /* CONFIG_USB */

static unsigned int
pl_pcibios_config_access(unsigned char access_type, struct pci_dev *dev,
                           unsigned char where, unsigned int data)
{
	unsigned char bus = dev->bus->number;
	unsigned char dev_fn = dev->devfn;

    /*
     * PL1061 doesn't process Host bridge command, we must fake
     * Hostbridge by software. The device number of all device
     * under hostbridge must shift 1 number.
     */

    if (bus == 0 && dev_fn == 0) {   /* Host bridge */
        if (access_type == PCI_ACCESS_WRITE)
            return data;

        switch(where) {
        case PCI_VENDOR_ID:
            return (PROLIFIC_PCI_HOSTBRIDGE_ID << 16) | PROLIFIC_PCI_ID;
        case PCI_DEVICE_ID:
            return PROLIFIC_PCI_HOSTBRIDGE_ID;
        case PCI_STATUS:
            return PCI_STATUS_CAP_LIST | PCI_STATUS_FAST_BACK | PCI_STATUS_REC_MASTER_ABORT;
        case PCI_HEADER_TYPE:
            return PCI_HEADER_TYPE_BRIDGE;
        case PCI_CLASS_REVISION:
            return (PCI_CLASS_BRIDGE_HOST << 16) | PROLIFIC_PCI_HOSTBRIDGE_REVISION;
        case PCI_CLASS_DEVICE:
            return PCI_CLASS_BRIDGE_HOST;
        default:
            return 0;
        }
    }

#ifdef CONFIG_USB
	if (bus == 0 &&
		PCI_SLOT(dev_fn) == PL_PCI_USB_DEVICE &&
		PCI_FUNC(dev_fn) == 0)
		return pl_pcibios_usb_access(access_type, dev, where, data);
#endif


    if (bus == 0)
        dev_fn = PCI_DEVFN(PCI_SLOT(dev_fn) - 1, PCI_FUNC(dev_fn));

	/* Setup address */
    wordAddr(PL_PCI0_CFGADDR_OFS) =
		 (bus         << PCI0_CFGADDR_BUSNUM_SHF)   |
		 (dev_fn      << PCI0_CFGADDR_FUNCTNUM_SHF) |
		 ((where / 4) << PCI0_CFGADDR_REGNUM_SHF)   |
		 PCI0_CFGADDR_CONFIGEN_BIT;

	if (access_type == PCI_ACCESS_WRITE) {
        wordAddr(PL_PCI0_CFGDATA_OFS) = data;
	} else {
        data = wordAddr(PL_PCI0_CFGDATA_OFS);
	}

	return data;
}


/*
 * We can't address 8 and 16 bit words directly.  Instead we have to
 * read/write a 32bit word and mask/modify the data we actually want.
 */
static int
pl_pcibios_read_config_byte (struct pci_dev *dev, int where, unsigned char *val)
{
	unsigned int data;

	data = pl_pcibios_config_access(PCI_ACCESS_READ, dev, where, 0);
	*val = (data >> ((where & 3) << 3)) & 0xff;
	return PCIBIOS_SUCCESSFUL;
}


// static int
int
pl_pcibios_read_config_word (struct pci_dev *dev, int where, unsigned short *val)
{
	unsigned int data;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;

	data = pl_pcibios_config_access(PCI_ACCESS_READ, dev, where, 0);
	*val = (data >> ((where & 3) << 3)) & 0xffff;

	return PCIBIOS_SUCCESSFUL;
}

/* PLAY TRICK
 * pl1061 allow to configure i/o to be 32-bit address to pci device, but
 * the pci hostbridge will mask the high-order 16-bit that cause pci
 * device failed to decode the i/o address.  So, when the pci device
 * set the i/o address, here mask the high order 16-bit.  And, when
 * the pci device read the i/o address back, we add the high-order 16-bit
 * to fake 32-bit i/o addressing.
 *  
 * wrire_config_dword(dev, BASE_ADDRESS_0, 0x1a000000) -> 0x0000
 * read_config_dword(dev, BASE_ADDRESS_0, &l)  -> l = KSEG1ADDR(0x1a000000)
 */

static int
pl_pcibios_read_config_dword (struct pci_dev *dev, int where, unsigned int *val)
{
	unsigned int data;

	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;
	
	data = pl_pcibios_config_access(PCI_ACCESS_READ, dev, where, 0);
    if (where == PCI_BASE_ADDRESS_0 || where == PCI_BASE_ADDRESS_1 ||
        where == PCI_BASE_ADDRESS_2 || where == PCI_BASE_ADDRESS_3 ||
        where == PCI_BASE_ADDRESS_4 || where == PCI_BASE_ADDRESS_5) {
        if ((data & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
              data = data | PL_PCI_IO_BASE;
         }
    }

	*val = data;

	return PCIBIOS_SUCCESSFUL;
}


static int
pl_pcibios_write_config_byte (struct pci_dev *dev, int where, unsigned char val)
{
	unsigned int data;
      
	data = pl_pcibios_config_access(PCI_ACCESS_READ, dev, where, 0);


	data = (data & ~(0xff << ((where & 3) << 3))) |
	       (val << ((where & 3) << 3));

	pl_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, data);
	return PCIBIOS_SUCCESSFUL;
}

static int
pl_pcibios_write_config_word (struct pci_dev *dev, int where, unsigned short val)
{
    unsigned int data;

	if (where & 1)
		return PCIBIOS_BAD_REGISTER_NUMBER;
       
    data = pl_pcibios_config_access(PCI_ACCESS_READ, dev, where, 0);
	data = (data & ~(0xffff << ((where & 3) << 3))) | 
	       (val << ((where & 3) << 3));

	pl_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, data);
	return PCIBIOS_SUCCESSFUL;
}

static int
pl_pcibios_write_config_dword(struct pci_dev *dev, int where, unsigned int val)
{
	if (where & 3)
		return PCIBIOS_BAD_REGISTER_NUMBER;

    if (val != ~0) {
        if (where == PCI_BASE_ADDRESS_0 || where == PCI_BASE_ADDRESS_1 ||
            where == PCI_BASE_ADDRESS_2 || where == PCI_BASE_ADDRESS_3 ||
            where == PCI_BASE_ADDRESS_4 || where == PCI_BASE_ADDRESS_5) {
            if ((val & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_IO) {
                val = val & PL_PCI_IO_MASK;
            }
            else{
                val = PHYS_IO_ADDRESS(val);
            }    
        }
    }
	pl_pcibios_config_access(PCI_ACCESS_WRITE, dev, where, val);
	return PCIBIOS_SUCCESSFUL;
}

struct pci_ops pl_pci_ops = {
	pl_pcibios_read_config_byte,
	pl_pcibios_read_config_word,
	pl_pcibios_read_config_dword,
	pl_pcibios_write_config_byte,
	pl_pcibios_write_config_word,
	pl_pcibios_write_config_dword
};

static void __init pl_pcibios_scan(void);


void __init pcibios_init(void)
{
    /* reset ioport_resource & iomem_resource for PL special addressing */
    ioport_resource.start = PL_PCI_IO_START;
    ioport_resource.end = PL_PCI_IO_END;
    request_mem_region((unsigned long)bus_to_virt(PL_PCI_IO_START), 
                        PL_PCI_IO_END - PL_PCI_IO_START + 1, 
                        "PL PCI IO map");
 
    /* allocate i/o & memory to pci slot */
    pl_pcibios_scan();
	printk("PCI: Probing PCI hardware on host bus 0.\n");
	pci_scan_bus(0, &pl_pci_ops, NULL);
}

int __init pcibios_enable_device(struct pci_dev *dev , int mask)
{
	/* Not needed, since we enable all devices at startup.  */
	return 0;
}

// void __init
// pcibios_align_resource(void *data, struct resource *res, unsigned long size)
void pcibios_align_resource(void *data, struct resource *res,
			    unsigned long size, unsigned long size2)
{
}

char * __init
pcibios_setup(char *str)
{
	/* Nothing to do for now.  */

	return str;
}

struct pci_fixup pcibios_fixups[] = {
	{ 0 }
};

void __init
pcibios_update_resource(struct pci_dev *dev, struct resource *root,
                        struct resource *res, int resource)
{
	unsigned long where, size;
	u32 reg;

	where = PCI_BASE_ADDRESS_0 + (resource * 4);
	size = res->end - res->start;
	pci_read_config_dword(dev, where, &reg);
	reg = (reg & size) | (((u32)(res->start - root->start)) & ~size);
	pci_write_config_dword(dev, where, reg);
}

/*
 *  Called after each bus is probed, but before its children
 *  are examined.
 */
void __init pcibios_fixup_bus(struct pci_bus *b)
{
	pci_read_bridge_bases(b);
}

int pci_dma_supported(struct pci_dev *pdev, dma_addr_t device_mask)
{
    return 0;
}

/*
 * pl_pcibios_scan: The pcibios_scan init pci devices alloc io and memory base
 *  address to each device.  It also assigned IRQ to each functions.
 */

#define PL_PCI_MAX_RESOURCE     128

#ifdef SHARE_MEMORY_RESOURCE
#define PL_PCI_IO_RESOURCE      1
#else
#define PL_PCI_IO_RESOURCE      4
#endif
static int ifree_res = PL_PCI_IO_RESOURCE + 1;

struct resource slot_res[PL_PCI_MAX_RESOURCE] __initdata = {
#ifdef SHARE_MEMORY_RESOURCE
    {"MEMORY", PL_PCI_MEM_START, PL_PCI_MEM_END, IORESOURCE_MEM | IORESOURCE_PREFETCH,  NULL, NULL, NULL},
#else
    {"SLOT1", 0x1a000000, 0x1a400000-1, IORESOURCE_MEM | IORESOURCE_PREFETCH,  NULL, NULL, NULL},
    {"SLOT2", 0x1a400000, 0x1a800000-1, IORESOURCE_MEM | IORESOURCE_PREFETCH,  NULL, NULL, NULL},
    {"SLOT3", 0x1a800000, 0x1ac00000-1, IORESOURCE_MEM | IORESOURCE_PREFETCH,  NULL, NULL, NULL},
    {"SLOT4", 0x1ac00000, 0x1b000000-1, IORESOURCE_MEM | IORESOURCE_PREFETCH,  NULL, NULL, NULL},
#endif
     {"IO",   PL_PCI_IO_START, PL_PCI_IO_END, IORESOURCE_IO,  NULL, NULL, NULL},
 };

static void __init pl_pcibios_set_irq(struct pci_dev *dev)
{
    unsigned char irq;
    pl_pcibios_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq);
    if (irq == 0)
        return;

    irq = ((128-(dev->device - 1) + (irq - 1))%4) + IRQ_PL_PCI_DEV0;

#ifdef PCI_SCAN_DEBUG
    printk("IRQ %d assigned to device %d:%d.%d\n",
        irq, dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
#endif

    pl_pcibios_write_config_byte(dev, PCI_INTERRUPT_LINE, irq);
    
}


#ifdef PCI_SCAN_DEBUG
static void dump_region(struct resource *root)
{
    unsigned int i;
    struct resource *p;
    printk("[%lx, %lx]->\n", root->start, root->end);
    for (p = root->child, i = 0; p != NULL && i < 5; p = p->sibling, i++)
        printk("(%lx, %lx) ", p->start, p->end);
    printk("\n");
}
#endif /* PCI_SCAN_DEBUG */

static int inline in_region(unsigned long pos, struct resource *res)
{
    return (pos >= res->start && pos <= res->end);
}


static int  __init __myrequest_region(struct resource *parent, 
        unsigned long start, unsigned long size, unsigned long align , 
        struct resource *res)
{
    int rc = -1;
    int staticaddr = 0;
    struct resource *tmp, **p;

    memset(res, 0, sizeof(*res));
    
    if (start == 0)
        start = parent->start;

    else
        staticaddr = 1;
    
    if (start < parent->start || (start + size) > parent->end)
        goto EXIT;

    res->start = start;
    res->end = start + size;
	res->flags = IORESOURCE_BUSY;

    p = &parent->child;
    while (res->end <= parent->end) {
	    tmp = *p;
	    if (!tmp || tmp->start > res->end) {
            res->sibling = tmp;
            *p = res;
            res->parent = parent;
            break;
	    } else if (tmp && 
                    (in_region(res->start, tmp) ||
                     in_region(res->end, tmp))) { 
            if (staticaddr)
                goto EXIT;
            res->start = (tmp->end + align) & -align;
            res->end = res->start + size;
        }

	    p = &tmp->sibling;
   	}

    if (res->end <= parent->end)
        rc = 0;

EXIT:
	return rc;
}

static int inline pl_pcibios_config_size(struct pci_dev *dev,
        unsigned int sz, unsigned int mask,
        unsigned int ires, unsigned int reg, unsigned int align,
        unsigned int l, struct resource *res, char *resname)
{
    unsigned int size;

    size = mask & sz;
    size = (size & ~(size -1)) - 1;

    if ( __myrequest_region(&slot_res[ires], 0, size, size, res)) {
        printk("failed to assigned %s to device: %d:%d.%d with size = %d\n",
            resname, dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn),
            size);
        return -1;
    }
    /* assigned memory resource to pci */
    l = (l & ~mask) | res->start;

#ifdef PCI_SCAN_DEBUG
    printk("%s resource [%lx~%lx] reg = %02x l =%x assigned to device %d:%d.%d\n",
        resname, res->start, res->end, reg, l, dev->bus->number,
        PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
#endif

    pl_pcibios_write_config_dword(dev, reg, l);
#ifdef PCI_SCAN_DEBUG
    pl_pcibios_read_config_dword(dev, reg, &l);
    printk("read back l = %x\n", l);
#endif


    return 0;
}

static void __init pl_pcibios_set_bases(struct pci_dev *dev, unsigned int howmany, int rom)
{
    unsigned int pos, reg, sz, l, next;
    struct resource *res;
    int rc;
    unsigned short command;

    pl_pcibios_read_config_word(dev, PCI_COMMAND, &command);

    res = &slot_res[ifree_res];
    for (pos = 0; pos < howmany; pos = next) {
        next = pos + 1;
        res->name = dev->name;
        reg = PCI_BASE_ADDRESS_0 + (pos << 2);
        pl_pcibios_read_config_dword(dev, reg, &l);
        pl_pcibios_write_config_dword(dev, reg, ~0);
        pl_pcibios_read_config_dword(dev, reg, &sz);
        pl_pcibios_write_config_dword(dev, reg, l);

        if (sz == 0 || sz == ~0)
            continue;

        if (l == ~0)
            l = 0;

#ifdef SHARE_MEMORY_RESOURCE
#define IRES  0
#else
#define IRES (dev->device -1)
#endif

        if ((l & PCI_BASE_ADDRESS_SPACE) == PCI_BASE_ADDRESS_SPACE_MEMORY) {
            rc = pl_pcibios_config_size(dev, sz, PCI_BASE_ADDRESS_MEM_MASK,
                    IRES, reg, 16, l, res, "Memory");
            if (rc < 0)
                return;
            res = &slot_res[++ifree_res];
            command |= PCI_COMMAND_MEMORY;
        } else { /* IO_MEMORY */
            rc = pl_pcibios_config_size(dev, sz, PCI_BASE_ADDRESS_IO_MASK, 
                    PL_PCI_IO_RESOURCE, reg, 4, l, res, "IO");
            if (rc < 0)
                return;
            res = &slot_res[++ifree_res];
            command |= PCI_COMMAND_IO;
        }

		if ((l & (PCI_BASE_ADDRESS_SPACE | PCI_BASE_ADDRESS_MEM_TYPE_MASK))
		    == (PCI_BASE_ADDRESS_SPACE_MEMORY | PCI_BASE_ADDRESS_MEM_TYPE_64)) {
            next++;
            printk("Unable to handle 64-bit address for device %d:%d.%d\n",
                dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));
            return;
        } 

    }

    if (rom) {
        res->name = dev->name;
        pl_pcibios_read_config_dword(dev, rom, &l);
        pl_pcibios_write_config_dword(dev, rom, ~PCI_ROM_ADDRESS_ENABLE);
        pl_pcibios_read_config_dword(dev, rom, &sz);
        pl_pcibios_write_config_dword(dev, rom, l);

        if (l == ~0)
            l = 0;
        if (sz && sz != ~0) {
            rc = pl_pcibios_config_size(dev, sz, PCI_ROM_ADDRESS_MASK,
                    IRES, rom, 16, l, res, "ROM");
            if (rc < 0)
                return;
            res = &slot_res[++ifree_res];
        }
    }

    pl_pcibios_write_config_word(dev, PCI_COMMAND, command);
}

static void __init pl_pcibios_scan_device(struct pci_dev *dev)
{
    unsigned int l;
    if (pl_pcibios_read_config_dword(dev, PCI_VENDOR_ID, &l))
        return;

    if (l == ~0 || l == 0 || l == 0xffff || l == (0xffff) << 16)
        return;

    /* setup irq, io, memory and rom base address */
    switch(dev->hdr_type) {
    case PCI_HEADER_TYPE_NORMAL:
        pl_pcibios_set_irq(dev);
        pl_pcibios_set_bases(dev, 6, PCI_ROM_ADDRESS);
        break;

    case PCI_HEADER_TYPE_BRIDGE:
        pl_pcibios_set_bases(dev, 2, PCI_ROM_ADDRESS1);
        break;

    case PCI_HEADER_TYPE_CARDBUS:
        pl_pcibios_set_irq(dev);
        pl_pcibios_set_bases(dev, 1, 0);
    }

}

static void __init pl_pcibios_scan_slot(struct pci_dev *dev)
{
    int is_multi = 0;
    unsigned int func;
    unsigned char hdr_type;    

    for(func = 0; func < 8 ; func++, dev->devfn++) {
        if (func && !is_multi)
            break;
        if (pl_pcibios_read_config_byte(dev, PCI_HEADER_TYPE, &hdr_type))
            continue;
        dev->hdr_type = hdr_type & 0x7f;

        pl_pcibios_scan_device(dev);
        if (!func) 
            is_multi = hdr_type & 0x80;
    }

}

static void __init pl_pcibios_scan_bus(struct pci_bus *bus)
{
    struct pci_dev dev0;
    unsigned int devfn;

    memset(&dev0, 0, sizeof(dev0));
    dev0.bus = bus;
    for(devfn = 0; devfn <= 0x100; devfn += 8) {
        dev0.devfn = devfn;
        if (bus->number == 0)
            dev0.device = PCI_SLOT(devfn);
        else
            dev0.device = bus->number;

        pl_pcibios_scan_slot(&dev0);
    }

    /* 
     * After peforming arch-dependent fixup of the bus, look behind all
     * PCI-to-PCI bridges on this bus.
     */
#if 0
    /* not implement yet !! */
#endif    

}


static void __init pl_pcibios_scan(void)
{
    struct pci_bus bus;

    memset(&bus, 0, sizeof(bus));
    __myrequest_region(&slot_res[PL_PCI_IO_RESOURCE], VIRT_IO_ADDRESS(0x1b800cf0), 16, 16, &slot_res[ifree_res++]);
#ifdef PCI_SCAN_DEBUG
    printk("PCI Config Set I/O Resource start = %lx end = %lx\n", 
        slot_res[ifree_res-1].start, slot_res[ifree_res-1].end);
#endif
    pl_pcibios_scan_bus(&bus);

#ifdef PCI_SCAN_DEBUG
printk("System halt for pci scan debugging\n");
while(1);
#endif

}


#endif /* CONFIG_PCI */
