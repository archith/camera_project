/*
 *  linux/arch/arm/mach-p52/arch.c
 *
 *  Architecture specific fixups.  This is where any
 *  parameters in the params struct are fixed up, or
 *  any additional architecture specific information
 *  is pulled from the params struct.
 */
#include <linux/config.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/blk.h>
#include <linux/version.h>

#include <asm/hardware.h>
#include <asm/irq.h>
#include <asm/setup.h>
#include <asm/mach-types.h>
#include <asm/mach/map.h>
#include <asm/mach/arch.h>
#include <asm/io.h>
#include <asm/pgtable.h>

/*
 * Chip specific mappings shared by all pl1029 systems
 */
#ifdef FINE_MAP_IO
static struct map_desc pl1029_sys_io_desc[] __initdata = {
    {rSYS_MANAGE_BASE,      rSYS_MANAGE_BASE_PHY,   rSYS_MANAGE_SIZE,   DOMAIN_IO, 0, 1 },
    {rCMC_BASE,             rCMC_BASE_PHY,          rCMC_SIZE,          DOMAIN_IO, 0, 1 },
    {rDMAC0_BASE,           rDMAC0_BASE_PHY,        rDMAC0_SIZE,        DOMAIN_IO, 0, 1 },
    {rPCI_IOPORT_BASE,      rPCI_IOPORT_BASE_PHY,   rPCI_IOPORT_SIZE,   DOMAIN_IO, 0, 1 },
    LAST_DESC,
};

static struct map_desc pl1029_peripherial_io_desc[] __initdata = {
    {rLCD_BASE,             rLCD_BASE_PHY,          rLCD_SIZE,          DOMAIN_IO, 0, 1 },
    {rUSB11_BASE,           rUSB11_BASE_PHY,        rUSB11_SIZE,        DOMAIN_IO, 0, 1 },
    {rUSB20_BASE,           rUSB20_BASE_PHY,        rUSB20_SIZE,        DOMAIN_IO, 0, 1 },
    {rAUDIO_OB_BASE,        rAUDIO_OB_BASE_PHY,     rAUDIO_OB_SIZE,     DOMAIN_IO, 0, 1 },
    {rAUDIO_IB_BASE,        rAUDIO_IB_BASE_PHY,     rAUDIO_IB_SIZE,     DOMAIN_IO, 0, 1 },
    {rAUDIO_CMD_BASE,       rAUDIO_CMD_BASE_PHY,    rAUDIO_CMD_SIZE,    DOMAIN_IO, 0, 1 },
    {rAUDIO_STATUS_BASE,    rAUDIO_STATUS_BASE_PHY, rAUDIO_STATUS_SIZE, DOMAIN_IO, 0, 1 },
    {rI2C_BASE,             rI2C_BASE_PHY,          rI2C_SIZE,          DOMAIN_IO, 0, 1 },
    {rIDE2_BASE,            rIDE2_BASE_PHY,         rIDE2_SIZE,         DOMAIN_IO, 0, 1 },
    {rNAND_BASE,            rNAND_BASE_PHY,         rNAND_SIZE,         DOMAIN_IO, 0, 1 },
    {rSD_BASE,              rSD_BASE_PHY,           rSD_SIZE,           DOMAIN_IO, 0, 1 },
    {rCF_BASE,              rCF_BASE_PHY,           rCF_SIZE,           DOMAIN_IO, 0, 1 },
    {rMSPRO_BASE,           rMSPRO_BASE_PHY,        rMSPRO_SIZE,        DOMAIN_IO, 0, 1 },
    {rUART_BASE,            rUART_BASE_PHY,         rUART_SIZE,         DOMAIN_IO, 0, 1 },
    {rMMX_LFB2PHB_BASE,     rMMX_LFB2PHB_BASE_PHY,  rMMX_LFB2PHB_SIZE,  DOMAIN_IO, 0, 1 },
    {rMMX_LMAPORT_BASE,     rMMX_LMAPORT_BASE_PHY,  rMMX_LMAPORT_SIZE,  DOMAIN_IO, 0, 1 },
    {rMMX_VPP_BASE,         rMMX_VPP_BASE_PHY,      rMMX_VPP_SIZE,      DOMAIN_IO, 0, 1 },
    {rMMX_PHP2LFB_BASE,     rMMX_PHP2LFB_BASE_PHY,  rMMX_PHP2LFB_SIZE,  DOMAIN_IO, 0, 1 },
    {rMMX_VDE_BASE,         rMMX_VDE_BASE_PHY,      rMMX_VDE_SIZE,      DOMAIN_IO, 0, 1 },
    LAST_DESC,
};
#else
static struct map_desc pl1029_io_desc[] __initdata = {
    { __phys_to_virt(0x18000000), 0x18000000,   (0x20000000 - 0x18000000), DOMAIN_IO, 0, 1 },
    LAST_DESC,
};
#endif

#if 1   /* for low level debug only. must map eb000000 before invokation */
void ll_puts(const char *s)
{
    for (; *s != '\0' ; s++) {
        if (*s == '\n') {
            while( (__raw_readb(0xdb000400) & 0x02)); /* until not full */
            __raw_writeb('\r', 0xdb000401);
        }
        while( (__raw_readb(0xdb000400) & 0x02)); /* until not full */
        __raw_writeb(*s, 0xdb000401);
    }
}

int ll_printk(const char *fmt, ...)
{
    va_list args;
    char buf[1024];
    int i;

    va_start(args, fmt);
    i = vsprintf(buf, fmt, args); /* hopefully i < sizeof(buf)-4 */
    va_end(args);

    ll_puts(buf);

    return i;
}
#endif


static void __init pl1029_map_io(void)
{
#ifdef FINE_MAP_IO
    iotable_init(pl1029_sys_io_desc);
    iotable_init(pl1029_peripherial_io_desc);
#else
    iotable_init(pl1029_io_desc);
#endif
}


void pl_machine_power_off(void)
{
    writew(0xffff, PL_OPMODE_SW+2); /* System shutdown write toggle */
} /* pl_machine_power_off() */


void pl_machine_reset(void)
{
    writew(0xffff, PL_OPMODE_SW2+2);/* System reset write toggle */
} /* pl_machine_power_off() */


unsigned long DRAM_SIZE = 0;

extern void (*pm_power_off)(void);
extern void plser_register_uart(int idx, int port);

static void __init fixup_PL1029(struct machine_desc *desc,
                                struct param_struct *params,
                                char **cmdline, struct meminfo *mi)
{
    ROOT_DEV = MKDEV(RAMDISK_MAJOR, 0);
#ifdef CONFIG_BLK_DEV_RAM_SIZE
    setup_ramdisk(1, 0, 0, CONFIG_BLK_DEV_RAM_SIZE);
#endif
    pm_power_off = pl_machine_power_off;

    plser_register_uart(0, 1);  /* com port */

    printk("Prolific arm arch version %s\n", ARCH_VERSION);
}

/* defined in mach-pl1029/irq.c */
extern void __init pl_init_irq(void);

MACHINE_START(PL1029, "Prolific ARM9v4 - PL1029")
       MAINTAINER("Jedy Wei")
       BOOT_MEM(0x00000000, 0x1b000000, 0xdb000000)
       BOOT_PARAMS(0x00002000)
       FIXUP(fixup_PL1029)
       MAPIO(pl1029_map_io)
       INITIRQ(pl_init_irq)
MACHINE_END




