/*
 * addr implementation for busybox
 *
 * Copyright (C) 2003-2004 by Jedy Wei
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <utmp.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <linux/ioctl.h>
#include <sys/mman.h>
#include "busybox.h"


static int iofd = 0;
static unsigned long iooffset = 0;

int io_init(void)
{
    if ((iofd = open("/dev/plio", O_RDWR)) < 0) {
        printf("Failed to open device /dev/plio");
        return -1;
    }

    /* mapp plio to virtual memory space */
    iooffset = (unsigned long) mmap(0, 0x20000000 - 0x18000000,
            PROT_READ | PROT_WRITE,  MAP_SHARED, iofd, 0);

    iooffset = 0xd8000000 - iooffset;

    return 0;
}

int io_release(void)
{
    if (iofd != 0)
        close(iofd);
    iofd = 0;

    return 0;
}


#define readb(addr) (*(volatile unsigned char *)(addr-iooffset))
#define readw(addr) (*(volatile unsigned short *)(addr-iooffset))
#define readl(addr) (*(volatile unsigned int *)(addr-iooffset))

#define writeb(b,addr) (*(volatile unsigned char *)(addr-iooffset)) = (b)
#define writew(b,addr) (*(volatile unsigned short *)(addr-iooffset)) = (b)
#define writel(b,addr) (*(volatile unsigned int *)(addr-iooffset)) = (b)


void usage(void)
{
    printf("USAGE: addr [-1|-2|-4] -a address [-v value]\n");
}


static int read_cmd(int len, int argc, char **argv)
{
    unsigned int addr;

    if (argc <= 1) {
        printf("Usage: read[4|2|1] register\n");
        goto EXIT;
    }

    if (io_init() < 0) {
        printf("Failed to open helper port\n");
        goto EXIT;
    }

    addr = strtoul(argv[1], NULL, 16);

    switch(len) {
    case 4:
        printf("%08X=%08x\n", addr, readl(addr));
        break;
    case 2:
        printf("%08X=%04x\n", addr, readw(addr));
        break;
    case 1:
        printf("%08X=%02x\n", addr, readb(addr));
        break;
    }



EXIT:
    io_release();

    return 0;

}
extern int read4_main(int argc, char **argv)
{
    return read_cmd(4, argc, argv);
}

extern int read2_main(int argc, char **argv)
{
    return read_cmd(2, argc, argv);
}

extern int read1_main(int argc, char **argv)
{
    return read_cmd(1, argc, argv);
}

static int write_cmd(int len, int argc, char **argv)
{
    unsigned int addr;
    unsigned int val;

    if (argc <= 2) {
        printf("Usage: write[4|2|1] register value\n");
        goto EXIT;
    }

    if (io_init() < 0) {
        printf("Failed to open helper port\n");
        goto EXIT;
    }

    addr = strtoul(argv[1], NULL, 16);
    val  = strtoul(argv[2], NULL, 0);

    switch(len) {
    case 4:
        printf("%08X=%08x\n", addr, val);
        writel(val, addr);
        break;
    case 2:
        printf("%08X=%04x\n", addr, (val & 0xffff));
        writew(val, addr);
        break;
    case 1:
        printf("%08X=%02x\n", addr, (val & 0xff));
        writeb(val, addr);
        break;
    }


EXIT:
    io_release();

    return 0;
}

extern int write4_main(int argc, char **argv)
{
    return write_cmd(4, argc, argv);
}

extern int write2_main(int argc, char **argv)
{
    return write_cmd(2, argc, argv);
}

extern int write1_main(int argc, char **argv)
{
    return write_cmd(1, argc, argv);
}


extern int addr_main(int argc, char **argv)
{
    int c;
    unsigned int addr = 0;
    unsigned int addr2 = 0;
    char *p;
    unsigned int val = 0;
    int vflag = 0;
    int dflag = 0;
    int size = 4;

    for (;;) {
        c = getopt (argc, argv, "124a:v:hd:");
        if (c == EOF)
            break;

        switch(c) {
        case 'a':
            addr = strtoul(optarg, NULL, 16);
            break;
        case 'v':
            val = strtoul(optarg, NULL, 0);
            vflag++;
            break;
        case 'd':
            addr = strtoul(optarg, &p, 16);
            p++; /* skip '-' */
            addr2 = strtoul(optarg, &p, 16);
            dflag++;
            break;

        case '1':
            size = 1;
            break;
        case '2':
            size = 2;
            break;
        case '4':
            size = 4;
            break;
        case 'h':
            usage();
            goto EXIT;
        }
    }
    if (addr == 0) {
        usage();
        goto EXIT;
    }

    if (io_init() < 0) {
        printf("Failed to open helper port\n");
        goto EXIT;
    }

    if (dflag && addr2 > 0) {
    }


    switch (size) {
    case 1:
        if (vflag) {
            printf("%08X=%02x\n", addr, (val & 0xff));
            writeb(val, addr);
        } else {
            printf("%08X=%02x\n", addr, readb(addr));
        }
        break;
    case 2:
        if (vflag) {
            printf("%08X=%04x\n", addr, (val & 0xffff));
            writew(val, addr);
        } else {
            printf("%08X=%04x\n", addr, readw(addr));
        }
        break;
    case 4:
        if (vflag) {
            printf("%08X=%08x\n", addr, val);
            writel(val, addr);
        } else {
            printf("%08X=%08x\n", addr, readl(addr));
        }
        break;
    }

EXIT:
    io_release();

    return 0;
}
