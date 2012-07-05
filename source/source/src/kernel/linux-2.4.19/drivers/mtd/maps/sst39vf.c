/*
 * Flash on CPE sst39vf
 *
 * $Id: sst39vf.c,v 1.1.1.1 2006/03/13 10:29:36 jedy Exp $
 */

#include <linux/module.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <asm/arch/io.h>
#include <asm/io.h>
#include <asm/arch/hardware.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/map.h>
#include <linux/mtd/partitions.h>


#define NB_OF(x)  (sizeof(x)/sizeof(x[0]))

#define FLASH_START			0x80400000
#define FLASH_SIZE			(4*1024*1024)
#define FLASH_WIDTH			4


__u8 sst39vf_read8(struct map_info *map, unsigned long ofs)
{
	return __raw_readb(map->map_priv_1 + ofs);
}

__u16 sst39vf_read16(struct map_info *map, unsigned long ofs)
{
	return __raw_readw(map->map_priv_1 + ofs);
}

__u32 sst39vf_read32(struct map_info *map, unsigned long ofs)
{
	return __raw_readl(map->map_priv_1 + ofs);
}

void sst39vf_write8(struct map_info *map, __u8 d, unsigned long adr)
{
	__raw_writeb(d, map->map_priv_1 + adr);
	mb();
//	udelay(10);
}

void sst39vf_write16(struct map_info *map, __u16 d, unsigned long adr)
{
	__raw_writew(d, map->map_priv_1 + adr);
	mb();
//	udelay(10);
}

void sst39vf_write32(struct map_info *map, __u32 d, unsigned long adr)
{
	__raw_writel(d, map->map_priv_1 + adr);
	mb();
//	udelay(10);
}

void sst39vf_copy_from(struct map_info *map, void *to, unsigned long from, ssize_t len)
{
	// printk ("sst39vf_copy_from: 0x%x@0x%x -> 0x%x\n", len, from, to);
	//memcpy_fromio(to, map->map_priv_1 + from, len);
	memcpy(to, (void *)(map->map_priv_1 + from), len);
//	udelay(10);
}

void sst39vf_copy_to(struct map_info *map, unsigned long to, const void *from, ssize_t len)
{
	while(len) {
		__raw_writeb(*(unsigned char *) from, map->map_priv_1 + to);
		from++;
		to++;
		len--;
	}
//	udelay(10);
}


static struct mtd_info *flash_mtd;

struct map_info sst39vf_flash_map = {
	name: "flash",
	size: FLASH_SIZE,
	buswidth: FLASH_WIDTH,
	read8: sst39vf_read8,
	read16: sst39vf_read16,
	read32: sst39vf_read32,
	copy_from: sst39vf_copy_from,
	write8: sst39vf_write8,
	write16: sst39vf_write16,
	write32: sst39vf_write32,
	copy_to: sst39vf_copy_to
};

struct resource sst39vf_flash_resource = {
	name:   "Flash",
	start:  FLASH_START,
	end:    FLASH_START + FLASH_SIZE - 1,
	flags:  IORESOURCE_IO | IORESOURCE_BUSY,
};

static struct mtd_partition sst39vf_flash_partition[] = {
	{ name: "all",		offset: 0x0, 		size: 0x400000 }
};

static int __init init_sst39vf_flash (void)
{
	int err;
	
	if (request_resource (&ioport_resource, &sst39vf_flash_resource)) 
	{
		printk(KERN_NOTICE "Failed to reserve sst39vf FLASH space\n");
		err = -EBUSY;
		goto out;
	}
	
	sst39vf_flash_map.map_priv_1 = (unsigned long)ioremap(FLASH_START, FLASH_SIZE);
	if (!sst39vf_flash_map.map_priv_1) 
	{
		printk(KERN_NOTICE "Failed to ioremap sst39vf FLASH space\n");
		err = -EIO;
		goto out_resource;
	}

	flash_mtd = do_map_probe("jedec_probe", &sst39vf_flash_map);
#ifdef not_complete_yet
	if (!flash_mtd) {
		flash_mtd = do_map_probe("map_rom", &sst39vf_flash_map);
		if (flash_mtd)
			flash_mtd->erasesize = 0x10000;
	}
#endif /* end_of_not */
	if (!flash_mtd) {
		printk("FLASH probe failed\n");
		err = -ENXIO;
		goto out_ioremap;
	}

	flash_mtd->module = THIS_MODULE;

#ifdef not_complete_yet	
	if (add_mtd_device(flash_mtd)) {
		printk("FLASH device addition failed\n");
		err = -ENOMEM;
		goto out_probe;
	}
#endif /* end_of_not */
	if (add_mtd_partitions(flash_mtd, sst39vf_flash_partition, NB_OF(sst39vf_flash_partition)))
	{
		printk("FLASH device addition failed\n");
		err = -ENOMEM;
		goto out_probe;
	}

	return 0;

out_probe:
	map_destroy(flash_mtd);
	flash_mtd = 0;
out_ioremap:
	iounmap((void *)sst39vf_flash_map.map_priv_1);
out_resource:
	release_resource (&sst39vf_flash_resource);
out:
	return err;
}




#ifdef not_complete_yet
static struct mtd_info *sram_mtd;

struct map_info sst39vf_sram_map = {
	name: "SRAM",
	size: SRAM_SIZE,
	buswidth: SRAM_WIDTH,
	read8: sst39vf_read8,
	read16: sst39vf_read16,
	read32: sst39vf_read32,
	copy_from: sst39vf_copy_from,
	write8: sst39vf_write8,
	write16: sst39vf_write16,
	write32: sst39vf_write32,
	copy_to: sst39vf_copy_to
};

struct resource sst39vf_sram_resource = {
	name:   "SRAM",
	start:  SRAM_START,
	end:    SRAM_START + SRAM_SIZE - 1,
	flags:  IORESOURCE_IO | IORESOURCE_BUSY,
};

static int __init init_sst39vf_sram (void)
{
	int err;
	
	if (request_resource (&ioport_resource, &sst39vf_sram_resource)) {
		printk(KERN_NOTICE "Failed to reserve sst39vf SRAM space\n");
		err = -EBUSY;
		goto out;
	}
	
	sst39vf_sram_map.map_priv_1 = (unsigned long)ioremap(SRAM_START, SRAM_SIZE);
	if (!sst39vf_sram_map.map_priv_1) {
		printk(KERN_NOTICE "Failed to ioremap sst39vf SRAM space\n");
		err = -EIO;
		goto out_resource;
	}

	sram_mtd = do_map_probe("map_ram", &sst39vf_sram_map);
	if (!sram_mtd) {
		printk("SRAM probe failed\n");
		err = -ENXIO;
		goto out_ioremap;
	}

	sram_mtd->module = THIS_MODULE;
	sram_mtd->erasesize = 16;
	
	if (add_mtd_device(sram_mtd)) {
		printk("SRAM device addition failed\n");
		err = -ENOMEM;
		goto out_probe;
	}
		
	return 0;

out_probe:
	map_destroy(sram_mtd);
	sram_mtd = 0;
out_ioremap:
	iounmap((void *)sst39vf_sram_map.map_priv_1);
out_resource:
	release_resource (&sst39vf_sram_resource);
out:
	return err;
}

#endif /* end_of_not */




#ifdef not_complete_yet
static struct mtd_info *bootrom_mtd;

struct map_info sst39vf_bootrom_map = {
	name: "BootROM",
	size: BOOTROM_SIZE,
	buswidth: BOOTROM_WIDTH,
	read8: sst39vf_read8,
	read16: sst39vf_read16,
	read32: sst39vf_read32,
	copy_from: sst39vf_copy_from,
};

struct resource sst39vf_bootrom_resource = {
	name:   "BootROM",
	start:  BOOTROM_START,
	end:    BOOTROM_START + BOOTROM_SIZE - 1,
	flags:  IORESOURCE_IO | IORESOURCE_BUSY,
};


static int __init init_sst39vf_bootrom (void)
{
	int err;
	
	if (request_resource (&ioport_resource, &sst39vf_bootrom_resource)) {
		printk(KERN_NOTICE "Failed to reserve sst39vf BOOTROM space\n");
		err = -EBUSY;
		goto out;
	}
	
	sst39vf_bootrom_map.map_priv_1 = (unsigned long)ioremap(BOOTROM_START, BOOTROM_SIZE);
	if (!sst39vf_bootrom_map.map_priv_1) {
		printk(KERN_NOTICE "Failed to ioremap sst39vf BootROM space\n");
		err = -EIO;
		goto out_resource;
	}

	bootrom_mtd = do_map_probe("map_rom", &sst39vf_bootrom_map);
	if (!bootrom_mtd) {
		printk("BootROM probe failed\n");
		err = -ENXIO;
		goto out_ioremap;
	}

	bootrom_mtd->module = THIS_MODULE;
	bootrom_mtd->erasesize = 0x10000;
	
	if (add_mtd_device(bootrom_mtd)) {
		printk("BootROM device addition failed\n");
		err = -ENOMEM;
		goto out_probe;
	}
		
	return 0;

out_probe:
	map_destroy(bootrom_mtd);
	bootrom_mtd = 0;
out_ioremap:
	iounmap((void *)sst39vf_bootrom_map.map_priv_1);
out_resource:
	release_resource (&sst39vf_bootrom_resource);
out:
	return err;
}
#endif /* end_of_not */


void flash_dummy()
{
	int i;
	
	printk("enter flash_dummy\n");
}

static int __init init_sst39vf_maps(void)
{
#ifdef not_complete_yet
	printk(KERN_INFO "CPE sst39vf MTD mappings:\n  Flash 0x%x at 0x%x\n  SRAM 0x%x at 0x%x\n  BootROM 0x%x at 0x%x\n", 
	       FLASH_SIZE, FLASH_START, SRAM_SIZE, SRAM_START, BOOTROM_SIZE, BOOTROM_START);
#endif /* end_of_not */


	init_sst39vf_flash();
	
#ifdef not_complete_yet
	init_sst39vf_sram();
	init_sst39vf_bootrom();
#endif /* end_of_not */

	return 0;
}
	

static void __exit cleanup_sst39vf_maps(void)
{
#ifdef not_complete_yet
	if (sram_mtd) {
		del_mtd_device(sram_mtd);
		map_destroy(sram_mtd);
		iounmap((void *)sst39vf_sram_map.map_priv_1);
		release_resource (&sst39vf_sram_resource);
	}
#endif /* end_of_not */

	if (flash_mtd) {
		//del_mtd_device(flash_mtd);
		del_mtd_partitions(flash_mtd);
		map_destroy(flash_mtd);
		iounmap((void *)sst39vf_flash_map.map_priv_1);
		release_resource (&sst39vf_flash_resource);
	}

#ifdef not_complete_yet
	if (bootrom_mtd) {
		del_mtd_device(bootrom_mtd);
		map_destroy(bootrom_mtd);
		iounmap((void *)sst39vf_bootrom_map.map_priv_1);
		release_resource (&sst39vf_bootrom_resource);
	}
#endif /* end_of_not */
}

module_init(init_sst39vf_maps);
module_exit(cleanup_sst39vf_maps);

MODULE_AUTHOR("lmc83");
MODULE_DESCRIPTION("ARM sst39vf map driver");
MODULE_LICENSE("GPL");
