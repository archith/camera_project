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

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ioport.h>
#include <linux/pci.h>

#include <asm/hardware.h>


/*
 * definitions
 */

#define CLK_MAIN_OSCI_CONFIG_MASK       ((0x7) << 24)
#define CCR_MAIN(x)                      (((x) & (CLK_MAIN_OSCI_CONFIG_MASK)) >> 24)
#define CCR_MPLLEN(x)                   ((x) & (1 << 11))
#define CCR_PLLMRANGE(x)                ((x) & (1 << 14))
#define CCR_N(x)                        ((x) & 0x3f)
#define CCR_M(x)                        (((x) >> 6) & 0x1f)
#define CCR_SYSCLK_SRC(x)               ((x) & (1 << 23))
#define CCR_PCI_BRIDGE(x)               ((x) & (1 << 20))

#define CCR_EPCICD(x)                   (((x) >> 16) & (0xf))
#define CCR_USBCD(x)                    (((x) >> 28) & 0x7)
#define CCR_PCIDESKEW(x)                ((x) & (1 << 15))
#define CCR_PCIDESKEW_RANGE(x)          ((x) & (1 << 13))
#define CCR_EXTPCI(x)                   ((x) & (1 << 27))
#define CCR_PCIDRIVE(x)                 ((x) & (1 << 12))

#define CCR3_CPUCD(x)                   (((x) >> 12) & 0xf)
#define CCR3_MEMCD(x)                   (((x) >> 8) & 0xf)
#define CCR3_DEVCD(x)                   ((x) & 0xf)
#define CCR3_PCICD(x)                   (((x) >> 4) & 0xf)
/*
 * Various clock divider types
 */
#define CD_CPU		0
#define CD_PCI		1
#define CD_DEV		2
#define CD_USB		3
#define CD_MEM          5
#define CD_EPCI		6


/* default clock is 12MHz */
#define DEFAULT_CLK	    (24*1000)


/*
 * variables
 */
static unsigned int osci_freq = DEFAULT_CLK;    /* unit kHz */
static unsigned int xclk_in;    /* unit Hz */
static unsigned int pll_out;    /* unit Hz */



/*
 * forward referneces
 */
static unsigned int pl_get_clock(int which);
static void	    pl_set_clock(int which, unsigned int cd);


/*
 * get clock function unit kHz
 */
unsigned int
pl_get_cpu_clock(void)
{
    return pl_get_clock(CD_CPU)/1000;
} /* pl_get_cpu_clock() */

unsigned int
pl_get_pci_clock(void)
{
    return pl_get_clock(CD_PCI)/1000;
} /* pl_get_pci_clock() */

unsigned int
pl_get_epci_clock(void)
{
    return pl_get_clock(CD_EPCI)/1000;
} /* pl_get_pci_clock() */

unsigned int
pl_get_dev_clock(void)
{
    return pl_get_clock(CD_DEV)/1000;
} /* pl_get_dev_clock() */


unsigned int
pl_get_usb_clock(void)
{
    return pl_get_clock(CD_USB)/1000;
} /* pl_get_usb_clock() */

unsigned int
pl_get_mem_clock(void)
{
    return pl_get_clock(CD_MEM)/1000;
} /* pl_get_usb_clock() */


/*
 * get clock function unit Hz
 */

unsigned int
pl_get_cpu_hz(void)
{
    return pl_get_clock(CD_CPU);
} /* pl_get_cpu_clock() */

unsigned int
pl_get_pci_hz(void)
{
    return pl_get_clock(CD_PCI);
} /* pl_get_pci_clock() */

unsigned int
pl_get_epci_hz(void)
{
    return pl_get_clock(CD_EPCI);
} /* pl_get_pci_clock() */

unsigned int
pl_get_dev_hz(void)
{
    return pl_get_clock(CD_DEV);
} /* pl_get_dev_clock() */


unsigned int
pl_get_usb_hz(void)
{
    return pl_get_clock(CD_USB);
} /* pl_get_usb_clock() */

unsigned int
pl_get_mem_hz(void)
{
    return pl_get_clock(CD_MEM);
} /* pl_get_usb_clock() */



#define div_clk_cd(xclk, cd) ((cd == 15) ? ((xclk) * 2 / 3) : ((xclk) / cd))

static unsigned int
pl_get_clock(int which)
{
    unsigned int ccr, ccr3, cd = 1;
    unsigned int clk;
    unsigned int xout;

    ccr  = readl(PL_CLK_CFG);
    ccr3 = readl(PL_CLK_CFG3);

    if (CCR_MPLLEN(ccr)) {
        xout = pll_out;
    } else {
        xout = xclk_in;
    }

    switch (which) {
    case CD_CPU:
        cd =  CCR3_CPUCD(ccr3) + 1;
        clk = div_clk_cd(xout, cd);

        return clk;

    case CD_MEM:
        if (CCR_SYSCLK_SRC(ccr)) {
            cd =  CCR3_MEMCD(ccr3) + 1;
            clk = div_clk_cd(xout, cd);
        } else {
            cd =  CCR3_CPUCD(ccr3) + 1;
            clk = div_clk_cd(xout, cd);
        }

        return clk;

    case CD_PCI:
        if (CCR_PCI_BRIDGE(ccr)) {
            cd = CCR3_PCICD(ccr3) + 1;
            clk = div_clk_cd(xout, cd);
        } else {
            cd = CCR_EPCICD(ccr) + 1;
            clk = div_clk_cd(xout, cd);
        }
	return clk;

    case CD_EPCI:
        cd = CCR_EPCICD(ccr) + 1;
        clk = div_clk_cd(xout, cd);

	return clk;

    case CD_DEV:
	cd = CCR3_DEVCD(ccr3) + 1;
        clk = div_clk_cd(xout, cd);
        return clk;

    case CD_USB:
        cd = CCR_USBCD(ccr) + 1;
        clk = div_clk_cd(xout, cd);

        return clk;
    }

    return 0;
} /* pl_get_clock() */

static void
pl_set_clock(int which, unsigned int cd)
{
    unsigned int ccr;

    ccr  = readl(PL_CLK_CFG);

    switch (which) {
    case CD_USB:
	if (cd > 8) cd = 8;
	    cd--;

        ccr &= ~(0x7 << 28);
        ccr |= (cd << 28);
        writel(ccr, PL_CLK_CFG);

	break;
    }
} /* pl_set_clock() */


static unsigned int
get_sub_dev_clk(unsigned int dclk, unsigned int uclk, unsigned int cd)
{
    switch(cd) {
    case 0:        return 0;
    case 1:        return uclk;
    case 2:        return uclk/2;
    case 3:        return uclk/4;
    case 4:        return dclk;
    case 5:        return dclk/2;
    case 6:        return dclk/4;
    case 7:        return dclk/8;
    }
    return 0;
}

static unsigned int
get_sub_dev_clk_uart(unsigned int dclk, unsigned int uclk, unsigned int cd)
{
    switch(cd) {
    case 0:        return 0;
    case 1:        return dclk;
    case 2:        return dclk/2;
    case 3:        return dclk/4;
    case 4:        return uclk;
    case 5:        return uclk/2;
    case 6:        return uclk/3;
    case 7:        return uclk/6;
    }
    return 0;
}

char *main_xtl_source[] =
{ "2M~12MHz", "24M~42MHz", "12M~24MHz", "Undef", "Ext Osc" };

int
get_hardware_list(char *buf)
{
    unsigned int   rom;
    int	       y, m, d;
    unsigned int   t;
    unsigned short md[13] = { 0, 31, 59, 90, 120, 151, 181, 212,
				  243, 273, 304, 334, 365 };
    unsigned int   ccr, ccr3;
    unsigned int   cpu, pci, epci;
    unsigned int   dev, usb, mem;
    int	       len = 0;
    unsigned int nand_cd, i2c_cd, ac97_cd, gpio_cd, ide1_cd, uart1_cd;
    unsigned int sd_cd, uart_cd, ms_cd, jmp4_cd, spi_cd, aes_cd, nor_cd;

    /*
     * ROM code version information
     */
    // rom = readl(rROM_VER);
    rom = 0;

    y = (rom / 365) + 1970;
    t = (y / 4) - (1970 / 4) - ((y / 100) - (1970 / 100)) +
    (y / 400) - (1970 / 400);
    if ((y % 4) == 0) md[2] += 1;

    t = (rom - t) % 365;
    for (m = 1; m < 13; m++) if (t < md[m]) break;

    d = t - md[m-1] + 1;

    /*
     * reading other configuration values
     */
    ccr = readl(PL_CLK_CFG);
    ccr3 = readl(PL_CLK_CFG3);

    /*
     * information about the clocking system
     */
    cpu = pl_get_cpu_hz();
    pci = pl_get_pci_hz();
    epci = pl_get_epci_hz();

    dev = pl_get_dev_hz();
    usb = pl_get_usb_hz();
    mem = pl_get_mem_hz();

    /* Rom Version */
    len += sprintf(buf, "ROM Ver:   %d/%d/%d\n\n", y,m,d);

    /* Routing Detail */
    len += sprintf(buf+len, "[Routing Detail]\n"
        "Source: %dMHz\n"
        "clk range: %s\n"
        "multiply_on: %s\n"
        "pllm_range: %s\n"
        "pllout = xclk_in * %d/%d (n/m)\n"
        "mem src = %s\n"
        "pci_bridge = %s\n\n",
            xclk_in/1000000,
            main_xtl_source[CCR_MAIN(ccr)],
           (CCR_MPLLEN(ccr)) ? "pllm" : "xclk_in",
           (CCR_PLLMRANGE(ccr)) ? "100M~300MHz" : "20M~100MHz",
           CCR_N(ccr), CCR_M(ccr),
           CCR_SYSCLK_SRC(ccr) ? "independ" : "lock with cpu",
           CCR_PCI_BRIDGE(ccr) ? "enabled" : "disabled" );

    /* Divider and Registers */
    len += sprintf(buf+len, "[Divider and Register]\n"
        "cfg_fdiv: %d  "
        "cfg_hdiv: %d  "
        "cfg_pdiv: %d\n"
        "cfg_ediv: %d  "
        "cfg_ddiv: %d  "
        "cfg_udiv: %d\n"
        "External PCI: %s\n"
        "PCI Deskew: %s (%s)\n"
        "PCI Drive: %dmA\n"
        "ccr:  %08x\n"
        "ccr3: %08x\n\n",
            CCR3_CPUCD(ccr3), CCR3_MEMCD(ccr3),
            CCR3_PCICD(ccr3), CCR_EPCICD(ccr), CCR3_DEVCD(ccr3), CCR_USBCD(ccr),
            CCR_EXTPCI(ccr) ? "enabled" : "disabled",
            CCR_PCIDESKEW(ccr) ? "enabled" : "disabled",
            CCR_PCIDESKEW_RANGE(ccr) ? "100M~300MHz" : "20M~100MHz",
            CCR_PCIDRIVE(ccr) ? 8 : 4,
            ccr, ccr3);

    /* Clock Detail */
    len += sprintf(buf+len, "[Clock Detail]\n"
        "XCLK_IN:    %8d kHz\n"
        "PLL_OUT:    %8d kHz\n"
        "CPU clock:  %8d kHz\n"
        "MEM clock:  %8d kHz\n"
        "PCI clock:  %8d kHz\n"
        "EPCI clock: %8d kHz\n"
        "DEV clock:  %8d kHz\n"
        "USB clock:  %8d kHz\n\n",
        /* "LCD clock:  %8d kHz\n\n", */
        xclk_in/1000, pll_out/1000,
        cpu/1000, mem/1000,
        pci/1000, epci/1000, dev/1000, usb/1000);


    nand_cd = readb(PL_CLK_NAND)& 0x7;
    i2c_cd = readb(PL_CLK_I2C)& 0x7;
    ac97_cd = readb(PL_CLK_AC97)& 0x7;
    gpio_cd = readb(PL_CLK_GPIO)& 0x7;
    ide1_cd = readb(PL_CLK_IDE1)& 0x7;
    sd_cd = readb(PL_CLK_SD)& 0x7;
    uart1_cd = readb(PL_CLK_UART_1)& 0x7;
    uart_cd = readb(PL_CLK_UART)& 0x7;
    ms_cd = readb(PL_CLK_MS)& 0x7;
    jmp4_cd = readb(PL_CLK_JMP4)& 0x7;
    spi_cd = readb(PL_CLK_SPI)& 0x7;
    aes_cd = readb(PL_CLK_AES)& 0x7;
    nor_cd = readb(PL_CLK_NOR)& 0x7;

    /* Device Clock Detail */
    len += sprintf(buf+len, "[Device Clock Detail]\n"
        "NAND:       %8d kHz\n"
        "I2C:        %8d kHz\n"
        "AC97:       %8d kHz\n"
        "GPIO:       %8d kHz\n"
        "IDE1:       %8d kHz\n"
        "SD:         %8d kHz\n"
        /* "PCCARD:     %8d kHz\n" */
        "GPIO2:      %8d kHz\n"
        "UART:       %8d kHz\n"
        "MS:         %8d kHz\n"
        /* "IDE2:       %8d kHz\n\n" */
        "JMP4:       %8d kHz\n"
        "SPI:        %8d kHz\n"
        "AES:        %8d kHz\n"
        "NOR:        %8d kHz\n",
        get_sub_dev_clk(dev, usb, nand_cd)/1000,
        get_sub_dev_clk(dev, usb, i2c_cd)/1000,
        get_sub_dev_clk(dev, usb, ac97_cd)/1000,
        get_sub_dev_clk(dev, usb, gpio_cd)/1000,
        get_sub_dev_clk(dev, usb, ide1_cd)/1000,
        get_sub_dev_clk(dev, usb, sd_cd)/1000,
        /* get_sub_dev_clk(dev, pccard_cd)/1000, */
        get_sub_dev_clk_uart(dev, usb, uart1_cd)/1000,
        get_sub_dev_clk_uart(dev, usb, uart_cd)/1000,
        get_sub_dev_clk(dev, usb, ms_cd)/1000,
        /* get_sub_dev_clk(dev, ide2_cd)/1000, */
        get_sub_dev_clk(dev, usb, jmp4_cd)/1000,
        get_sub_dev_clk(dev, usb, spi_cd)/1000,
        get_sub_dev_clk(dev, usb, aes_cd)/1000,
        get_sub_dev_clk(dev, usb, nor_cd)/1000);


	return len;
} /* get_hardware_list() */

int get_malloc(char *buf)
{
    return 0;
}

static int __init
init_osci_freq(void)
{
    unsigned int ccr;
    unsigned int ns, ms;


    ccr = readl(PL_CLK_CFG);
    xclk_in = osci_freq * 1000;

    /* calculate clock out */
    if (CCR_MPLLEN(ccr)) { /* pll enable */
        ns = CCR_N(ccr);
        ms = CCR_M(ccr);
        pll_out = xclk_in * ns / ms;
    } else {
        pll_out = xclk_in;
    }

    return 0;
}

module_init(init_osci_freq);

static int __init
osci_freq_setup(char *str)
{
    int	     f = 0;
    unsigned int ccr;

    /*
     * to get input clock frequency on MXTL_IO
     */
    if (str != (char *) 0) {
        for (;*str != '\0';str++) f = f * 10 + (*str - '0');
            osci_freq = (f == 0) ? osci_freq : f;
    }

    return 1;
} /* osci_freq_setup() */

__setup("osci_freq=", osci_freq_setup);
