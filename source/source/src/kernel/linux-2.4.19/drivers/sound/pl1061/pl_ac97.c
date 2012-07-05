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
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/pci.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>
#include <asm/dma.h>
#include <asm/irq.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/ac97_codec.h>
#include <linux/major.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/delay.h>
#include <asm/arch-pl1029/pl_symbol_alarm.h>


#define VERSION "2.2.2"

/* Use DEBUG defination to enable debugging message */
#define xDEBUG


#if  defined(CONFIG_PL1063) || defined(CONFIG_ARCH_PL1029)
#define AUDIO_MODULE_63     1
#define AUDIO_MODULE_63_DMA_LINE_BUG            1
#define AUDIO_MODULE_63_LINE_RESIDUE_BUG        1
#define AUDIO_MODULE_TRIM_DSP_WRITE_TO_DWORD    1
#define AUDIO_MODULE_63_WINDOW_BUG              1
#else
#define AUDIO_MODULE_61     1
#endif

/******************************************************************************
 * Prolific AC'97/I2S/SPDIF Module Regsiter Definition
 ******************************************************************************/

#define PL_AUDIO_BASE_IO_PORT       0xd9480000
#define PL_AUDIO_OB                 (PL_AUDIO_BASE_IO_PORT + 0x00000)
#define PL_AUDIO_IB                 (PL_AUDIO_BASE_IO_PORT + 0x10000)
#define PL_AUDIO_CMD                (PL_AUDIO_BASE_IO_PORT + 0x20000)
#define PL_AUDIO_STATUS             (PL_AUDIO_BASE_IO_PORT + 0x30000)

/* CMD Register Set */
#define PL_AUDIO_CMD_COMMAND_0      (PL_AUDIO_CMD + 0x00)
#define PL_AUDIO_CMD_COMMAND_1      (PL_AUDIO_CMD + 0x01)
#define PL_AUDIO_CMD_COMMAND_2      (PL_AUDIO_CMD + 0x02)
#define PL_AUDIO_CMD_COMMAND_3      (PL_AUDIO_CMD + 0x03)
#define PL_AUDIO_CMD_FUNCTION_0     (PL_AUDIO_CMD + 0x04 + 0x00)
#define PL_AUDIO_CMD_FUNCTION_1     (PL_AUDIO_CMD + 0x04 + 0x01)
#define PL_AUDIO_CMD_FUNCTION_2     (PL_AUDIO_CMD + 0x04 + 0x02)
#define PL_AUDIO_CMD_FUNCTION_3     (PL_AUDIO_CMD + 0x04 + 0x03)
#define PL_AUDIO_CMD_AC97CONFIG_0   (PL_AUDIO_CMD + 0x08 + 0x00)
#define PL_AUDIO_CMD_AC97CONFIG_1   (PL_AUDIO_CMD + 0x08 + 0x01)
#define PL_AUDIO_CMD_AC97CONFIG_2   (PL_AUDIO_CMD + 0x08 + 0x02)
#define PL_AUDIO_CMD_AC97CONFIG_3   (PL_AUDIO_CMD + 0x08 + 0x03)
#define PL_AUDIO_CMD_AC97ADDR       (PL_AUDIO_CMD + 0x0c)
#define PL_AUDIO_CMD_AC97WDATA      (PL_AUDIO_CMD + 0x10)
#define PL_AUDIO_CMD_AC97STATUS     (PL_AUDIO_CMD + 0x14)
#define PL_AUDIO_CMD_AC97RDATA      (PL_AUDIO_CMD + 0x18)
#define PL_AUDIO_CMD_AC97CLOCK      (PL_AUDIO_CMD + 0x1c)
#define PL_AUDIO_CMD_GPIO_C_ENB     (PL_AUDIO_CMD + 0x20 + 0x00)
#define PL_AUDIO_CMD_GPIO_C_DO      (PL_AUDIO_CMD + 0x20 + 0x01)
#define PL_AUDIO_CMD_GPIO_C_OE      (PL_AUDIO_CMD + 0x24)
#define PL_AUDIO_CMD_GPIO_C_DRV_E2  (PL_AUDIO_CMD + 0x24 + 0x01)
#define PL_AUDIO_CMD_GPIO_C_DRV_E4  (PL_AUDIO_CMD + 0x24 + 0x02)
#define PL_AUDIO_CMD_GPIO_C_DRV_E8  (PL_AUDIO_CMD + 0x24 + 0x03)
#define PL_AUDIO_CMD_GPIOPULL       (PL_AUDIO_CMD + 0x28)
#define PL_AUDIO_CMD_GPIOIN         (PL_AUDIO_CMD + 0x2c)

#define PL_AUDIO_CMD_DOWNPACK       (PL_AUDIO_CMD + 0x38)
#define PL_AUDIO_CMD_UPPACK         (PL_AUDIO_CMD + 0x3c)

#if AUDIO_MODULE_63
#define PL_AUDIO_STATUS_OUTCOUNT    (PL_AUDIO_STATUS + 0x0e)
#define PL_AUDIO_STATUS_INCOUNT     (PL_AUDIO_STATUS + 0x0c)
#else
#define PL_AUDIO_CMD_OUTCOUNT       (PL_AUDIO_CMD + 0x84)
#define PL_AUDIO_CMD_INCOUNT        (PL_AUDIO_CMD + 0x80)
#endif

/* CMD's Command Register */
#if AUDIO_MODULE_63
#define COMMAND_3_ENBO              (1 << 7)    /* Enable audio output */
#define COMMAND_3_AMUO_MASK         (7)
#define COMMAND_2_ENBI              (1 << 7)    /* Enable audio input */
#define COMMAND_2_AMUI_MASK         (7)
#define COMMAND_1_B_RSTN            (1 << 7)    /* Audio Controller Interal Reset */
#define COMMAND_1_INTE_DNDA         (1 << 3)
#define COMMAND_1_INTE_DNSA         (1 << 2)
#define COMMAND_1_INTE_UPDA         (1 << 1)
#define COMMAND_1_INTE_UPSA         (1 << 0)

#define COMMAND_0_PAUSE             (1 << 7)
#define COMMAND_0_STOP              (1 << 6)

#else /* CONFIG_PL1061 */
#define COMMAND_3_ENBO              (1 << 7)    /* Enable audio output */
#define COMMAND_3_DMAD              (1 << 6)    /* Enable down-stream DMA */
#define COMMAND_2_ENBI              (1 << 7)    /* Enable audio input */
#define COMMAND_2_DMAU              (1 << 6)    /* Enable up-stream DMA */
#define COMMAND_1_B_RSTN            (1 << 7)    /* Audio Controller Interal Reset */
#define COMMAND_0_INTE              (1 << 4)    /* Interrupt enable */
#endif /* CONFIG_PL1061 */

#define COMMAND_0_MODE_MASK         0x03        /* Mode Mask */
#define COMMAND_0_SPDIF             0x01        /* Mode selete: SPDIF */
#define COMMAND_0_AC97              0x02        /*              AC97 */
#define COMMAND_0_I2S               0x03        /*              I2S */


/* CMD's Function Register */
#define FUNCTION_3_SP_ST            (1 << 4)    /* SPDIF record starting preamble */
#define FUNCTION_3_SP_V_CHK         (1 << 3)    /* SPDIF valid bit check enable */
#define FUNCTION_3_SP_P_CHK         (1 << 2)    /* SPDIF parity bit check enable */
#define FUNCTION_3_SP_I_FS_MASK     0x03        /* SPDIF up stream frequency MASK */
#define FUNCTION_3_SP_I_FS_48K      0x00        /* frequency : 48kHz */
#define FUNCTION_3_SP_I_FS_44K      0x01        /* frequency : 44.1kHz */
#define FUNCTION_3_SP_I_FS_32K      0x02        /* frequency : 32kHz */

#define FUNCTION_1_A_RSTN           (1 << 7)    /* AC97 Codec Reset */
#define FUNCTION_1_VRA              (1 << 0)    /* AC97 Codec various frequency enable */

#define FUNCTION_0_D_SIGN           (1 << 4)    /* Signed bit invert for down stream */
#define FUNCTION_0_U_SIGN           (1 << 0)    /* Signed bit invert for up stream */


/* CMD's AC97 Codec Configuration Register */
#define AC97CONFIG_3_AUSM_MASK      (0x3 << 6)  /* up stream stereo/mono */
#define AC97CONFIG_3_AUSM_NORMAL    (0x0 << 6)  /* 00 normal */
#define AC97CONFIG_3_AUSM_STEREO    (0x1 << 6)  /* 01 stereo */
#define AC97CONFIG_3_AUSM_MONO_LEFT (0x2 << 6)  /* 10 mono left */
#define AC97CONFIG_3_AUSM_MONO_RIGHT (0x3 << 6) /* 11 mono right */

#define AC97CONFIG_3_AUPS_MASK      (0x3 << 4)  /* up stream channel precision */
#define AC97CONFIG_3_AUPS_20        (0x0 << 4)  /* 00: 20bits/channel */
#define AC97CONFIG_3_AUPS_18        (0x1 << 4)  /* 01: 18bits/channel */
#define AC97CONFIG_3_AUPS_16        (0x2 << 4)  /* 10: 16bits/channel */
#define AC97CONFIG_3_AUPS_8         (0x3 << 4)  /* 11: 8bits/channel */


#define AC97CONFIG_2_ADSM_MASK      (0x3 << 6)  /* down stream stereo/mono */
#define AC97CONFIG_2_ADSM_NORMAL    (0x0 << 6)  /* 00 normal (stereo) */
#define AC97CONFIG_2_ADSM_STEREO    (0x0 << 6)  /* 00 normal (stereo) */
#define AC97CONFIG_2_ADSM_MONO      (0x1 << 6) /* 01 mono right (slot4) */
#define AC97CONFIG_2_ADSM_MONO_RIGHT (0x1 << 6) /* 01 mono right (slot4) */
#define AC97CONFIG_2_ADSM_MONO_LEFT (0x2 << 6)  /* 10 mono left (slot3) */


#define AC97CONFIG_2_ADPS_MASK      (0x3 << 4)  /* down stream channel precision */
#define AC97CONFIG_2_ADPS_20        (0x0 << 4)  /* 00: 20bits/channel */
#define AC97CONFIG_2_ADPS_18        (0x1 << 4)  /* 01: 18bits/channel */
#define AC97CONFIG_2_ADPS_16        (0x2 << 4)  /* 10: 16bits/channel */
#define AC97CONFIG_2_ADPS_8         (0x3 << 4)  /* 11: 8bits/channel */

/* CMD's AC97Clock register */
#define AC97CLOCK_CLKE              (1 << 0)    /* 1: external CODEC clock, 0: internal D_CLK */
#define AC97CLOCK_CLKD              (1 << 1)    /* 1: divider enable, 0: divider disable */


/* STATUS's Register */
#define STATUS_SEMA_ENBO            (1UL << 31) /* Down stream semaphore active */
#define STATUS_ENBO                 (1UL << 27) /* Down stream active */
#define STATUS_DS_INT               (1UL << 23) /* Down stream interrupt source, read clear */
#define STATUS_DS_OB_EMP            (1UL << 19) /* Down stream buffer empty flag */
#define STATUS_DS_OB_FUL            (1UL << 18) /* Down stream buffer full flag */
#define STATUS_DS_OB_OVF            (1UL << 17) /* Down stream buffer overflow flag */
#define STATUS_DS_OB_UNF            (1UL << 16) /* Down stream buffer underflow flag */

#define STATUS_SEMA_ENBI            (1UL << 15) /* Down stream semaphore active */
#define STATUS_ENBI                 (1UL << 11) /* Down stream active */
#define STATUS_US_INT               (1UL << 7)  /* Down stream interrupt source, read clear */
#define STATUS_US_IB_EMP            (1UL << 3)  /* Down stream buffer empty flag */
#define STATUS_US_IB_FUL            (1UL << 2)  /* Down stream buffer full flag */
#define STATUS_US_IB_OVF            (1UL << 1)  /* Down stream buffer overflow flag */
#define STATUS_US_IB_UNF            (1UL << 0)  /* Down stream buffer underflow flag */

/* GPIO Pin for AC97 */
#define GPIO_BIT_CLOCK              (1 << 0)   /* I/O */
#define GPIO_AC97_RSTN              (1 << 1)   /* O */
#define GPIO_AC97_SYNC              (1 << 2)   /* O */
#define GPIO_AC97_SO                (1 << 3)   /* O */
#define GPIO_AC97_SI                (1 << 4)   /* I */
#define GPIO_ALL_PIN                0x1f


#define DIR_OUT     1
#define DIR_IN      0

#define advance_idx(idx, up)    ({(idx)++; if ((idx) >= (up << 1)) (idx) = 0; })
#define regular_idx(idx, up)    (((idx) >= (up)) ? (idx)-(up) : (idx))
#define ABS(x)                  (((x) < 0) ? -(x) : (x))


/******************************************************************************
 *  Internal use structure
 ******************************************************************************/
static int DEFAULT_OUT_BUF_SLOT_SIZE_ORDER = PAGE_SHIFT;    /* 4096 */
static int DEFAULT_OUT_BUF_SLOTS = 8;
static int DEFAULT_IN_BUF_SLOT_SIZE_ORDER = PAGE_SHIFT;     /* 4096 */
static int DEFAULT_IN_BUF_SLOTS = 8;

#define MAX_SLOTS                   64

#define HW_FLAG_EMP     (1 << 3)
#define HW_FLAG_FUL     (1 << 2)
#define HW_FLAG_OVF     (1 << 1)
#define HW_FLAG_UNF     (1 << 0)

struct pl_audio_info {
    u32     slots;
    u32     runs;       /* for output it's underrun count, for input it's overrun count */
    u32     jam;        /* 2 issues is done under a interrupt */
    u32     flags;
    u32     losts;      /* interrupt disappear */
    u32     restores;   /* count restore from hardware error */
    u32     tbytes;         /* total bytes after beginning of open */
    u32     bbytes;         /* buffered bytes */
    u32     bytes_per_sec;  /* bytes per second */
    u32     bytes_per_hour; /* bytes per hour */
    u32     ptradjust;      /* ptr adjust */
};


struct dma_mde {
    u32     addr;
    u32     dcnt: 21;
    u32     nxt:  8;
    u32     j:    1;
    u32     v:    1;
    u32     e:    1;
};

#define ST_CLOSE    0
#define ST_OPEN     1
#define ST_RW       2

struct pl_audio_buf {
    int         state;                  /* ST_CLOSE, ST_OPEN, ST_RW */
    int         start;
    int         slots;
    int         order;                  /* the buffer size order, the buffer size = 1 << order */
    int         ihw;                    /* index to hardware current function slots */
    int         ihead;                  /* index to next ready mde slot */
    int         itail;                  /* index to next available (free) mde slot */
    atomic_t    issues;
    int         copys;                  /* the current slots copy from/to user buffer */
    int         threshold;              /* a slot should be filled at least size to send to device */
    struct dma_mde  mde[MAX_SLOTS] __attribute__ ((aligned (16)));    /* #008 */
    int         hw_error;               /* once hardware underflow, driver should wait DMA stopping */
    wait_queue_head_t   wait;           /* wait for buffer ready (read) or available (write) */
    wait_queue_head_t   drain;          /* wait for buffer drain out */
    int  (*issue)(struct pl_audio_buf *abuf);
    struct  pl_audio_info info;
    int         alarmpid;               /* pid for alarm */
    int         alarm;                  /* alarm active flag */
};


typedef struct pl_audio_tag {
    struct  ac97_codec  ac97_codec;
    int                 dspo;           /* dsp output device */
    int                 oused;
    struct pl_audio_buf out;
    int                 dspi;           /* dsp intput device */
    int                 iused;
    struct pl_audio_buf in;
} pl_audio_t;

static pl_audio_t pl_audio_ins;

/******************************************************************************
 *  PL Audio Codec I/O functions
 ******************************************************************************/
static unsigned long last_ac97_io = 0;

void pl_delay(unsigned long last_io)
{
    while (1) {
        if (time_before_eq(jiffies, last_io+1)) {
            if (!in_interrupt())
                schedule_timeout(1);
            else {
                udelay(50);
                return;
            }
        } else {
            return;
        }
    }
}


static u16 pl_codec_read(struct ac97_codec *codec, u8 addr)
{
    u32 dwData;
    pl_delay(last_ac97_io);

    last_ac97_io = jiffies;

    dwData = readl(PL_AUDIO_CMD_AC97WDATA);   /* clear new data flag, it's hardware's bug */
    writel((addr | 0x80) << 12, PL_AUDIO_CMD_AC97ADDR);

    /* do not wait more than 10 msec */
    do {
        dwData = readl(PL_AUDIO_CMD_AC97RDATA);
        if (dwData & (1UL << 31))   /* new data is ready */
            break;
    } while(time_before(jiffies, last_ac97_io+3));

    last_ac97_io = 0;
    return (u16) (dwData >> 4);
}

static void pl_codec_write(struct ac97_codec *codec, u8 addr, u16 data)
{
    pl_delay(last_ac97_io);

    writel(data << 4, PL_AUDIO_CMD_AC97WDATA);
    writel((addr & 0x7F) << 12, PL_AUDIO_CMD_AC97ADDR);
    last_ac97_io = jiffies;
}


/* codec_wait is used to wait for a ready state after an AC97_RESET */
static void pl_codec_wait(struct ac97_codec *codec)
{
    udelay(10);
}


/******************************************************************************
 *  downstream and upstream buffer mantain functions
 ******************************************************************************/
/**
 *      ihw ihead              itail
 *       v   v                   v
 *  +-------------------------------------+
 *  | | |*|*|.|.|.|.|.|.|.|.|.|.| | | | | |
 *  +-------------------------------------+
 *    issueing  ready but not issue
 */
static spinlock_t  issue_lock = SPIN_LOCK_UNLOCKED;
static int issue_out(struct pl_audio_buf *out)
{
    int flags = 0;
    int idx;
    struct pl_audio_info *out_info = &out->info;

    if (get_dma_residue(DMA_PL_AC97_OUT) == 0) {
        if (out->hw_error) {
            out->hw_error = 0;
            out_info->restores++;
        }
        disable_dma(DMA_PL_AC97_OUT);
        // out_info->runs++;
        wake_up(&out->drain);
    }

    if (!out->start)               /* forbidden by PAUSE or TRIGGER */
        return 4;

    if (out->ihead == out->itail)  /* EMPTY, no more ready slot */
        /* inactive dma mechine */
        return 1;

    /* check hardware error flag */
    if (out->hw_error)
        return 3;

    /* check hardware busy, if hw busy then just leaveing */
    if (atomic_read(&out->issues) >= 2)
        return 2;

    /**** Critical Section ****/
    if (! in_interrupt())
        spin_lock_irqsave(&issue_lock, flags);

    /* it should be checked again before issuing */
    if (out->ihead != out->itail &&
        !out->hw_error && atomic_read(&out->issues) < 2) {
        idx = regular_idx(out->ihead, out->slots);
#if AUDIO_MODULE_63_LINE_RESIDUE_BUG
        if (out->mde[idx].dcnt < 16)
            out->mde[idx].dcnt = 16;
#endif

        pci_map_single(NULL, bus_to_virt(out->mde[idx].addr), out->mde[idx].dcnt, PCI_DMA_TODEVICE);
        pci_map_single(NULL, out->mde, MAX_SLOTS * sizeof(struct dma_mde), PCI_DMA_TODEVICE);

#if AUDIO_MODULE_63
        writew(out->mde[idx].dcnt >> 2, PL_AUDIO_CMD_DOWNPACK);

#if     AUDIO_MODULE_63_WINDOW_BUG
        static unsigned int sdcnt = 0;
        static unsigned int scount = 0, hcount = 0, maxdcount = 0;
        int                 dcount = 0;
        unsigned int        enbo_start;

        while (dcount < 1000) {
            enbo_start = readb(PL_AUDIO_CMD_COMMAND_3);
            sdcnt = readw(PL_AUDIO_STATUS_OUTCOUNT);
            if ((enbo_start & 0x80) && sdcnt <= 2) {
                dcount++;
                udelay(25);
            } else {
                break;
            }
        }
#endif

#else
        writel(out->mde[idx].dcnt >> 2, PL_AUDIO_CMD_DOWNPACK);
#endif
        out->issues.counter++;
#if AUDIO_MODULE_63
        writeb(COMMAND_3_ENBO | 2, PL_AUDIO_CMD_COMMAND_3);
#else
        writeb(COMMAND_3_ENBO | COMMAND_3_DMAD, PL_AUDIO_CMD_COMMAND_3);
#endif


        /* release a mde slot */
        advance_idx(out->ihead, out->slots);

        /* active dma machine */
        if (get_dma_residue(DMA_PL_AC97_OUT) == 0) {
            set_dma_mde_idx(DMA_PL_AC97_OUT, idx);
            enable_dma(DMA_PL_AC97_OUT);
            out_info->runs++;
        }
    }

    if (! in_interrupt())
        spin_unlock_irqrestore(&issue_lock, flags);
    /**** End Critical Section ****/


    return 0;
}


/**
 *      ihead               ihw itail
 *       v                   v   v
 *  +-------------------------------------+
 *  | | |.|.|.|.|.|.|.|.|.|.|*|*| | | | | |
 *  +-------------------------------------+
 *       ready for copying     issuing
 */


static int issue_in(struct pl_audio_buf *in)
{
    int flags = 0;
    int idx;
    int i;
    struct pl_audio_info *in_info = &in->info;

    if (get_dma_residue(DMA_PL_AC97_IN) == 0 || atomic_read(&in->issues) == 0) {
        if (in->hw_error) {
            in->hw_error = 0;
            in_info->restores++;
        }
        // in_info->runs++;
        wake_up(&in->drain);
    }

    if (!in->start) /* forbidden by PAUSE or TRIGGER */
        return 4;

    if (ABS(in->itail-in->ihead) == in->slots)  /* FULL, no more available slot */
        return 1;

    /* check hardware error flag */
    if (in->hw_error)
        return 3;

    if (atomic_read(&in->issues) >= 2)
        return 2;

    /**** Critical Section ****/
    if (! in_interrupt())
        spin_lock_irqsave(&issue_lock, flags);

    for (i = 0; i < 2 -atomic_read(&in->issues); i++) {
        /* it should be check again before issuing */
        if (ABS(in->itail-in->ihead) != in->slots && !in->hw_error &&
            atomic_read(&in->issues) < 2) {
            idx = regular_idx(in->itail,in->slots);
            // pci_map_single(NULL, bus_to_virt(in->mde[idx].addr), in->mde[idx].dcnt, PCI_DMA_TODEVICE);
            pci_map_single(NULL, in->mde, MAX_SLOTS * sizeof(struct dma_mde), PCI_DMA_TODEVICE);

#if AUDIO_MODULE_63
            writew(in->mde[idx].dcnt >> 2, PL_AUDIO_CMD_UPPACK);
#else
            writel(in->mde[idx].dcnt >> 2, PL_AUDIO_CMD_UPPACK);
#endif
            in->issues.counter++;
#if AUDIO_MODULE_63
            writeb(COMMAND_2_ENBI | 2, PL_AUDIO_CMD_COMMAND_2);
#else
            writeb(COMMAND_2_ENBI | COMMAND_2_DMAU, PL_AUDIO_CMD_COMMAND_2);
#endif
            /* release a mde slot */
            advance_idx(in->itail, in->slots);

            /* active dma machine */
            if (get_dma_residue(DMA_PL_AC97_IN) == 0) {
                set_dma_mde_idx(DMA_PL_AC97_IN, idx);
                enable_dma(DMA_PL_AC97_IN);
                in_info->runs++;
            }
        }
    }

    if (! in_interrupt())
        spin_unlock_irqrestore(&issue_lock, flags);
    /**** End Critical Section ****/


    return 0;
}


static int drain_out(struct pl_audio_buf *out)
{
// printk("drain_out %d\n", get_dma_residue(DMA_PL_AC97_OUT));
    int idx;

    /* flush the last slot */
    idx = regular_idx(out->itail, out->slots);
    if (out->copys > 0) {
        out->mde[idx].dcnt = out->copys & ~3;  /* aligned to dword size */
    } else  {
        out->mde[idx].dcnt = 16;    /* add a junk line */
        memset((char *)bus_to_virt(out->mde[idx].addr), 0, 16);
    }
    out->copys = 0;
    out->mde[idx].v = 1;    /* valid mde entry */
    out->mde[idx].e = 1;    /* mark last mde */
    advance_idx(out->itail, out->slots);
    issue_out(out);

#if 1
    //wait_event(out->drain, get_dma_residue(DMA_PL_AC97_OUT) == 0);
    wait_event_interruptible(out->drain, get_dma_residue(DMA_PL_AC97_OUT) == 0);
#else
    if (get_dma_residue(DMA_PL_AC97_OUT) != 0)
        sleep_on_timeout(&out->drain, 2*HZ);
    if (get_dma_residue(DMA_PL_AC97_OUT) != 0) {
        printk("force drain_out\n");
#if AUDIO_MODULE_63
        writeb(2, PL_AUDIO_CMD_COMMAND_3);              /* force hardware to reset downstream */
#else
        writeb(COMMAND_3_DMAD, PL_AUDIO_CMD_COMMAND_3);  /* force hardware to reset downstream */
#endif
        wait_event_interruptible(out->drain, get_dma_residue(DMA_PL_AC97_OUT) == 0);
    }
#endif
#if AUDIO_MODULE_63
    writeb(2, PL_AUDIO_CMD_COMMAND_3);  /* clear DMAD flag */
#else
    writeb(0, PL_AUDIO_CMD_COMMAND_3);  /* clear DMAD flag */
#endif
// printk("after drain_out %d\n", get_dma_residue(DMA_PL_AC97_OUT));
    return 0;
}

static int drain_in(struct pl_audio_buf *in)
{
#if AUDIO_MODULE_63
    writeb(0, PL_AUDIO_CMD_COMMAND_2);
#else
    writeb(COMMAND_2_DMAU, PL_AUDIO_CMD_COMMAND_2);
#endif
    wait_event(in->drain, get_dma_residue(DMA_PL_AC97_IN) == 0);

#if 0
    int i;
    for (i = 0; i < 50; i++) {
        if (get_dma_residue(DMA_PL_AC97_IN) == 0)
            break;
        sleep_on_timeout(&in->drain, 2);
#if AUDIO_MODULE_63
        writeb(0, PL_AUDIO_CMD_COMMAND_2);
#else
        writeb(COMMAND_2_DMAU, PL_AUDIO_CMD_COMMAND_2);
#endif
    }
    if (get_dma_residue(DMA_PL_AC97_IN) != 0)
        printk("Failed to drain in (stop input)\n");
#endif

    writeb(0, PL_AUDIO_CMD_COMMAND_2);
    return 0;
}

static void inline start_issue(struct pl_audio_buf *abuf)
{
    abuf->start = 1;
    if (abuf->state == ST_RW)
        abuf->issue(abuf);
}

static void inline stop_issue(struct pl_audio_buf *abuf)
{
    abuf->start = 0;
}

static void pause_issue(struct pl_audio_buf *abuf)
{
    if (abuf->start)
        stop_issue(abuf);
    else
        start_issue(abuf);
}

static void inline clean_buffer(struct pl_audio_buf *abuf)
{
    abuf->ihw = abuf->itail = abuf->ihead = 0;
    atomic_set(&abuf->issues, 0);
    abuf->copys = 0;
    abuf->hw_error = 0;
}

/******************************************************************************
 *  interrupt handler
 ******************************************************************************/
static void pl_audio_handler(int irq, void *dev_id, struct pt_regs* regs)
{
    pl_audio_t *pl_audio = dev_id;
    struct pl_audio_buf *out = &pl_audio->out;
    struct pl_audio_buf *in = &pl_audio->in;
    struct pl_audio_info *out_info = &out->info;
    struct pl_audio_info *in_info = &in->info;
    int hw_out = 0, hw_in = 0;
    int i;
    u32 status = readl(PL_AUDIO_STATUS);
#ifdef DEBUG
    int oissues = atomic_read(&out->issues);
    int iissues = atomic_read(&in->issues);
#endif

    if (atomic_read(&out->issues) == 2) {
        if ((status & STATUS_SEMA_ENBO) == 0) {
            /* ac97 codec has comsumed a down stream slot */
            out->issues.counter--;
            hw_out++;
            out->mde[regular_idx(out->ihw, out->slots)].v = 0;   /* invalid mde entry */
            out_info->bbytes -= out->mde[regular_idx(out->ihw, out->slots)].dcnt;
            advance_idx(out->ihw, out->slots);
        }
    }

    if (atomic_read(&out->issues) == 1) {
        if ((status & STATUS_ENBO) == 0) {
            /* ac97 codec has comsumed a down stream slot */
            out->issues.counter--;
            hw_out++;
            out->mde[regular_idx(out->ihw, out->slots)].v = 0;   /* invalid mde entry */
            out_info->bbytes -= out->mde[regular_idx(out->ihw, out->slots)].dcnt;
            advance_idx(out->ihw, out->slots);
        }
    }

    if (atomic_read(&in->issues) == 2) {
        if ((status & STATUS_SEMA_ENBI) == 0) {
            /* ac97 codec has comsumed a up stream slot */
            in->issues.counter--;
            hw_in++;
            in_info->bbytes += in->mde[regular_idx(in->ihw, in->slots)].dcnt;
            advance_idx(in->ihw, in->slots);
        }
    }

    if (atomic_read(&in->issues) == 1) {
        if ((status & STATUS_ENBI) == 0) {
            /* ac97 codec has comsumed a up stream slot */
            in->issues.counter--;
            hw_in++;
            in_info->bbytes += in->mde[regular_idx(in->ihw, in->slots)].dcnt;
            advance_idx(in->ihw, in->slots);
        }
    }

#ifdef DEBUG
//     if (status & (STATUS_DS_OB_OVF | STATUS_DS_OB_UNF | STATUS_US_IB_OVF | STATUS_US_IB_UNF))

    printk("oissues %d (%d) iissues %d (%d) status = %08x hw_out %d (%d %d %d) hw_in %d (%d %d %d)\n",
            oissues, atomic_read(&out->issues),
            iissues, atomic_read(&in->issues),
            status, hw_out,
            out->ihw, out->ihead, out->itail,
            hw_in, in->ihead, in->ihw, in->itail);
#endif

    if (status & (STATUS_DS_OB_OVF | STATUS_DS_OB_UNF))
        out->hw_error++;

    if (status & (STATUS_US_IB_OVF | STATUS_US_IB_UNF))
        in->hw_error++;

    for (i = 0; i < hw_out; i++) {
        issue_out(out);
        wake_up_interruptible(&out->wait);
    }

    /* kill sigalarm */
    if (out->alarm && hw_out)
        kill_proc(out->alarmpid, SIGALRM, 1);


    for (i = 0; i < hw_in; i++) {
        issue_in(in);
        wake_up_interruptible(&in->wait);
    }

    /* calculate statistic information */
    if (hw_out == 2)
        out_info->jam++;

    if (hw_out && (status & STATUS_DS_INT) == 0)
        out_info->losts++;

    out_info->slots += hw_out;
    out_info->flags |= (status >> 16) & 0x0f;

    if (hw_in == 2)
        in_info->jam++;

    if (hw_in && (status & STATUS_US_INT) == 0)
        in_info->losts++;


    in_info->slots += hw_in;
    in_info->flags |= status & 0x0f;

}


/******************************************************************************
 *  Configure functions
 ******************************************************************************/
static void pl_dsp_set_channels(int channels, int output)
{
    u8 data;

    if (output) {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & ~AC97CONFIG_2_ADSM_MASK;
        if (channels == 2)
            writeb(data | AC97CONFIG_2_ADSM_STEREO, PL_AUDIO_CMD_AC97CONFIG_2);
        else
            writeb(data | AC97CONFIG_2_ADSM_MONO, PL_AUDIO_CMD_AC97CONFIG_2);
    } else {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & ~AC97CONFIG_3_AUSM_MASK;
        if (channels == 2)
            writeb(data | AC97CONFIG_3_AUSM_STEREO, PL_AUDIO_CMD_AC97CONFIG_3);
        else
            writeb(data | AC97CONFIG_3_AUSM_MONO_LEFT, PL_AUDIO_CMD_AC97CONFIG_3);
    }
}


static int pl_dsp_get_channels(int output)
{
    int channels;
    u8 data;

    if (output) {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & AC97CONFIG_2_ADSM_MASK;
        if (data == AC97CONFIG_2_ADSM_STEREO)
            channels = 2;
        else
            channels = 1;
    } else {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & AC97CONFIG_3_AUSM_MASK;
        if (data == AC97CONFIG_3_AUSM_STEREO)
            channels = 2;
        else
            channels = 1;
    }

    return channels;
}

static u16 pl_dsp_set_samplerate(struct ac97_codec *codec, u16 rate, int output)
{
    if (output) {
        pl_codec_write(codec, AC97_PCM_FRONT_DAC_RATE, rate);
        rate = pl_codec_read(codec, AC97_PCM_FRONT_DAC_RATE);
    } else {
        pl_codec_write(codec, AC97_PCM_LR_ADC_RATE, rate);
        rate = pl_codec_read(codec, AC97_PCM_LR_ADC_RATE);
    }
    return rate;
}

static u32 pl_dsp_get_samplerate(struct ac97_codec *codec, int output)
{
    u16 rate;
    if (output) {
        rate = pl_codec_read(codec, AC97_PCM_FRONT_DAC_RATE);
    } else {
        rate = pl_codec_read(codec, AC97_PCM_LR_ADC_RATE);
    }

    return rate;
}

static int pl_dsp_get_format(int output)
{
    int unsign = 0;
    int fmt;
    u8 data;

    if (output){
        unsign = readb(PL_AUDIO_CMD_FUNCTION_0) & FUNCTION_0_D_SIGN;
        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & AC97CONFIG_2_ADPS_MASK;
        fmt = (data == AC97CONFIG_2_ADPS_16) ? 16 : 8;
    } else {
        unsign = readb(PL_AUDIO_CMD_FUNCTION_0) & FUNCTION_0_U_SIGN;
        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & AC97CONFIG_3_AUPS_MASK;
        fmt = (data == AC97CONFIG_3_AUPS_16) ? 16 : 8;
    }

    if (! unsign) {
        if (fmt == 8)
            return AFMT_S8;
        else
            return AFMT_S16_LE;
    } else {
        if (fmt == 8)
            return AFMT_U8;
        else
            return AFMT_U16_LE;
    }

}

static int pl_dsp_set_format(int format, int output)
{
    int sign;
    int fmt;
    u8 data;

    switch(format) {
    case AFMT_QUERY:
        return pl_dsp_get_format(output);
    case AFMT_U8:
        sign = 0; fmt = 8;  break;
    case AFMT_S8:
        sign = 1; fmt = 8;  break;
    case AFMT_U16_LE:
        sign = 0; fmt = 16; break;
    case AFMT_S16_LE:
    default:
        sign = 1; fmt = 16;
        format = AFMT_S16_LE;
        break;
    }

    if (output){
        data = readb(PL_AUDIO_CMD_FUNCTION_0) & ~FUNCTION_0_D_SIGN;
        if (sign)
            writeb(data, PL_AUDIO_CMD_FUNCTION_0);
        else
            writeb(data | FUNCTION_0_D_SIGN, PL_AUDIO_CMD_FUNCTION_0);

        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & ~AC97CONFIG_2_ADPS_MASK;

        if (fmt == 8)
            writeb(data | AC97CONFIG_2_ADPS_8, PL_AUDIO_CMD_AC97CONFIG_2);
        else
            writeb(data | AC97CONFIG_2_ADPS_16, PL_AUDIO_CMD_AC97CONFIG_2);
    } else {
        data = readb(PL_AUDIO_CMD_FUNCTION_0) & ~FUNCTION_0_U_SIGN;
        if (sign)
            writeb(data, PL_AUDIO_CMD_FUNCTION_0);
        else
            writeb(data | FUNCTION_0_D_SIGN, PL_AUDIO_CMD_FUNCTION_0);

        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & ~AC97CONFIG_3_AUPS_MASK;

        if (fmt == 8)
            writeb(data | AC97CONFIG_3_AUPS_8, PL_AUDIO_CMD_AC97CONFIG_3);
        else
            writeb(data | AC97CONFIG_3_AUPS_16, PL_AUDIO_CMD_AC97CONFIG_3);
    }

    return format;
}

static void pl_calc_time_base(pl_audio_t *pl_audio, int output)
{
    int ch;
    int samples;
    int fmt;
    int bytes_per_sample;
    struct pl_audio_info *info;

    ch = pl_dsp_get_channels(output);
    samples = pl_dsp_get_samplerate(&pl_audio->ac97_codec, output);
    fmt = pl_dsp_get_format(output);
    if (fmt == AFMT_S16_LE || fmt == AFMT_U16_LE)
        bytes_per_sample = 2;
    else
        bytes_per_sample = 1;
    if (output)
        info = &pl_audio->out.info;
    else
        info = &pl_audio->in.info;

    info->bytes_per_sec = ch*bytes_per_sample*samples;
    info->bytes_per_hour = info->bytes_per_sec * 60 * 60;

}

/**
 * pl_dsp_getodelay: returns the number of unplayed bytes in the kernel buffer
 * NOTE: the hardware may play some samples which should be taken off
 */
static int pl_dsp_getodelay(struct pl_audio_buf *out)
{
    int size;
#if AUDIO_MODULE_63
    size = readw(PL_AUDIO_STATUS_OUTCOUNT) << 2;
#else
    size = readl(PL_AUDIO_CMD_OUTCOUNT) << 2;
#endif
    size = out->info.bbytes - size;
    if (size < 0)
        size = 0;

    return size;
}

/**
 * pl_dsp_getidelay: return the number of recoded bytes in the kernel buffer
 * NOTE: the hardware may recode some samples which should be counted
 */
static int pl_dsp_getidelay(struct pl_audio_buf *in)
{
    int size;

#if AUDIO_MODULE_63
    size = readw(PL_AUDIO_STATUS_INCOUNT) << 2;
#else
    size = readl(PL_AUDIO_CMD_INCOUNT) << 2;
#endif
    size = in->info.bbytes + size;
    if (size < 0)
        size = 0;

    return size;
}



static void pl_dsp_getoptr(struct pl_audio_buf *out, count_info *info)
{
    info->bytes = out->info.tbytes;
    info->blocks = out->info.slots;
    out->info.slots = 0;
    info->ptr = out->info.bbytes;
}

static void pl_dsp_getiptr(struct pl_audio_buf *in, count_info *info)
{
    info->bytes = in->info.tbytes;
    info->blocks = in->info.slots;
    in->info.slots = 0;
    info->ptr = in->info.bbytes;
}

#ifdef  CONFIG_PL_AC97_POWERDOWN
static void pl_dsp_set_powerdown_bit(struct ac97_codec *codec, u16 bit)
{
    u16 val = pl_codec_read(codec, AC97_POWER_CONTROL);
    val |= bit;
    pl_codec_write(codec, AC97_POWER_CONTROL, val & 0xFF00);
}


static void pl_dsp_clr_powerdown_bit(struct ac97_codec *codec, u16 bit)
{
    u16 val = pl_codec_read(codec, AC97_POWER_CONTROL);
    val &= ~bit;
    pl_codec_write(codec, AC97_POWER_CONTROL, val & 0xFF00);
}

static u32 pl_dsp_get_powerstatus(struct ac97_codec *codec)
{
    u16 status;
    status = pl_codec_read(codec, AC97_POWER_CONTROL);

    return status;
}

static void pl_dsp_set_mute(struct ac97_codec *codec, int mute)
{
    u16  master_vol=0, headphone_vol=0, mono_vol=0;

    if (mute) {
        master_vol = pl_codec_read(codec, AC97_MASTER_VOL_STEREO);
        headphone_vol = pl_codec_read(codec, AC97_HEADPHONE_VOL);
        mono_vol = pl_codec_read(codec, AC97_MASTER_VOL_MONO);

        /* Enable head-phone/line-out/mono-out mute */
        pl_codec_write(codec, AC97_MASTER_VOL_STEREO, master_vol|0x8000);
        pl_codec_write(codec, AC97_HEADPHONE_VOL, headphone_vol|0x8000);
        pl_codec_write(codec, AC97_MASTER_VOL_MONO, mono_vol|0x8000);
    } else {
        master_vol = pl_codec_read(codec, AC97_MASTER_VOL_STEREO);
        headphone_vol = pl_codec_read(codec, AC97_HEADPHONE_VOL);
        mono_vol = pl_codec_read(codec, AC97_MASTER_VOL_MONO);

        /* Disalbe head-phone/line-out/mono-out mute */
        pl_codec_write(codec, AC97_MASTER_VOL_MONO, mono_vol&0x7FFF);
        pl_codec_write(codec, AC97_HEADPHONE_VOL, headphone_vol&0x7FFF);
        pl_codec_write(codec, AC97_MASTER_VOL_STEREO, master_vol&0x7FFF);
    }
}

static void pl_dsp_power_up(pl_audio_t *pl_audio, int output)
{
    struct ac97_codec *codec = &pl_audio->ac97_codec;


    /* Be quiet */
    pl_dsp_set_mute(&pl_audio_ins.ac97_codec, 1);

    /* Powerup ADC/DAC */
    if (output)
        pl_dsp_clr_powerdown_bit(codec, AC97_PWR_PR1);
    else
        pl_dsp_clr_powerdown_bit(codec, AC97_PWR_PR0);

#ifdef  CONFIG_PL_AC97_POWERDOWN_MIXER
    /* Powerup mixer */
    pl_dsp_clr_powerdown_bit(codec, AC97_PWR_PR3);
#endif

    if (output) {
        /* Powerup headpone and EAPD */
        pl_dsp_clr_powerdown_bit(codec, AC97_PWR_PR6);
        pl_dsp_clr_powerdown_bit(codec, AC97_PWR_PR7);
    }

    /* Restore volumes */
    pl_dsp_set_mute(&pl_audio_ins.ac97_codec, 0);
}


static void pl_dsp_power_down(pl_audio_t *pl_audio, int output)
{
    struct ac97_codec *codec = &pl_audio->ac97_codec;
    u32     status = pl_dsp_get_powerstatus(codec);
    int     can_powerdown_mixer=0;


    pl_dsp_set_mute(&pl_audio_ins.ac97_codec, 1);

    if (output) {
        pl_dsp_set_powerdown_bit(codec, AC97_PWR_PR7);
        pl_dsp_set_powerdown_bit(codec, AC97_PWR_PR6);

#ifdef  CONFIG_PL_AC97_POWERDOWN_MIXER
        if (! (status&AC97_PWR_ADC))
            can_powerdown_mixer = 1;
    } else {
        if (! (status&AC97_PWR_DAC))
            can_powerdown_mixer = 1;
    }


    if (can_powerdown_mixer)
        pl_dsp_set_powerdown_bit(codec, AC97_PWR_PR3);
#else
    }
#endif

    if (output)
        pl_dsp_set_powerdown_bit(codec, AC97_PWR_PR1);
    else
        pl_dsp_set_powerdown_bit(codec, AC97_PWR_PR0);

    pl_dsp_set_mute(&pl_audio_ins.ac97_codec, 0);
}
#endif  /* CONFIG_PL_AC97_POWERDOWN */

static int pl_dsp_setfragment(struct pl_audio_buf *abuf, int valid, int nfrag, int frag_ord)
{
    int size;
    int i;
    u32 p = 0;

    if (nfrag > 64 || nfrag < 4)
        return -EINVAL;

    if (frag_ord > 15 || frag_ord < 8)
        return -EINVAL;

    /* don't change anything, if order, fragmets is same as original */
    if (abuf->order == frag_ord && abuf->slots == nfrag)
        return 0;


    /* release allocated buffer */
    if (abuf->order != 0) {
        if (abuf->order <= PAGE_SHIFT) {
            for (i = 0; i < abuf->slots; i+= (1 << (PAGE_SHIFT-abuf->order))) {
                free_page((u32)bus_to_virt(abuf->mde[i].addr));
            }
        } else {
            for (i = 0; i < abuf->slots; i++) {
                free_pages((u32)bus_to_virt(abuf->mde[i].addr), abuf->order - PAGE_SHIFT);
            }

        }
        abuf->order = 0;
    }

    /* allocate new buffer */
    size = 1 << frag_ord;
    for (i = 0; i < nfrag; i++) {
        if (frag_ord <= PAGE_SHIFT) {
            if ((i & ((1 << (PAGE_SHIFT-frag_ord)) - 1)) == 0) {
                p = abuf->mde[i].addr = (u32)virt_to_bus((void *)__get_free_page(GFP_KERNEL));
            } else {
                p += (1 << frag_ord);
                abuf->mde[i].addr = p;
            }
        } else {
            abuf->mde[i].addr = (u32) virt_to_bus((void *)__get_free_pages(GFP_KERNEL, frag_ord -PAGE_SHIFT));
        }
        abuf->mde[i].nxt = i+1;
        abuf->mde[i].e = 0;
        abuf->mde[i].v = valid;
        abuf->mde[i].dcnt = size;
    }

    abuf->mde[nfrag-1].nxt = 0;
    abuf->slots = nfrag;
    abuf->order = frag_ord;
    abuf->threshold = 1 << frag_ord;

    for (i = 0; i < abuf->slots; i++) {
        if (abuf->mde[i].addr == 0) {
            abuf->order = 0;
            return -ENOMEM;
        }
    }

    return 0;
}

static int get_order_by_size(int size)
{
    int i;

    for (i = 0; i < 30; i++)
        if ((1 << i) >= size)
            break;

    return i;
}


/******************************************************************************
 *  DSP functions
 ******************************************************************************/

static int pl_dsp_open(struct inode *inode, struct file *file)
{
    int fmt;
    int minor = MINOR(inode->i_rdev);

#if 0
    if ((file->f_mode & FMODE_WRITE) && (file->f_mode & FMODE_READ)) {
        printk("OSS doesn't support full deplux mode\n");
        return -EINVAL;
    }
#endif

    if ((minor & 0xf) == SND_DEV_DSP) {
        fmt = AFMT_U8;
    } else if ((minor & 0xf) == SND_DEV_DSP16) {
        fmt = AFMT_S16_LE;
    } else
        return -ENODEV;

    if ((file->f_mode & FMODE_WRITE) && pl_audio_ins.out.state != ST_CLOSE)
        return -EBUSY;

    if ((file->f_mode & FMODE_READ) && pl_audio_ins.in.state != ST_CLOSE)
        return -EBUSY;

    /* downstream */
    if (file->f_mode & FMODE_WRITE) {
        pl_audio_ins.out.state = ST_OPEN;
        pl_audio_ins.out.start = 1;
#ifdef  CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_up(&pl_audio_ins, 1);
#endif
        pl_dsp_set_channels(2, DIR_OUT);
        pl_dsp_set_format(fmt, DIR_OUT);
        pl_dsp_set_samplerate(&pl_audio_ins.ac97_codec,48000, DIR_OUT);
        memset(&pl_audio_ins.out.info, 0, sizeof(struct pl_audio_info));
        pl_calc_time_base(&pl_audio_ins, DIR_OUT);
    }

    /* upstream */
    if (file->f_mode & FMODE_READ) {
        pl_audio_ins.in.state = ST_OPEN;
        pl_audio_ins.in.start = 1;
#ifdef  CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_up(&pl_audio_ins, 0);
#endif
        pl_dsp_set_channels(2, DIR_IN);
        pl_dsp_set_format(fmt, DIR_IN);
        pl_dsp_set_samplerate(&pl_audio_ins.ac97_codec,8000, DIR_IN);
        memset(&pl_audio_ins.in.info, 0, sizeof(struct pl_audio_info));
        pl_calc_time_base(&pl_audio_ins, DIR_IN);
    }

    file->private_data = &pl_audio_ins;

    return 0;
}

static int pl_dsp_release(struct inode *inode, struct file *file)
{
    int rc1 = 0, rc2 = 0;
    pl_audio_t *pl_audio = (pl_audio_t *)file->private_data;

    if (file->f_mode & FMODE_WRITE) {
        rc1 = drain_out(&pl_audio->out);
        pl_audio->out.state = ST_CLOSE;
        clean_buffer(&pl_audio->out);
#ifdef  CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_down(pl_audio, 1);
#endif
    }

    if (file->f_mode & FMODE_READ) {
        rc2 = drain_in(&pl_audio->in);
        pl_audio->in.state = ST_CLOSE;
        clean_buffer(&pl_audio->in);
#ifdef  CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_down(pl_audio, 0);
#endif
    }

    if (rc1)
        return rc1;
    else
        return rc2;
}

static ssize_t pl_dsp_write(struct file *file, const char *buffer, size_t count, loff_t *ppos)
{
    int nwrite = 0;
    pl_audio_t *pl_audio = (pl_audio_t *)file->private_data;
    struct pl_audio_buf *out;
    int len, idx;
    int rc, bufsize;
    const char *p;

    if (pl_audio == NULL)
        return -EINVAL;

    out = &pl_audio->out;

    if (ppos != &file->f_pos)
        return -ESPIPE;

    if (!access_ok(VERIFY_READ, buffer, count))
        return -EFAULT;

#if AUDIO_MODULE_TRIM_DSP_WRITE_TO_DWORD
    count = count & ~3; /* trim to dword size */
#endif
    if ((count & 3) != 0)   /* should be double word size, limit by ac97 controller */
        return -EINVAL;

    out->state = ST_RW;

    start_issue(out);       /* #001 resume from pause state */

    bufsize = 1 << out->order;
    p = buffer;
    while (count > 0) {
        if (ABS(out->itail-out->ihw) == out->slots) { /* FULL, no more avaliable slot */
            if (file->f_flags & O_NONBLOCK)
                return (nwrite == 0) ? -EAGAIN : nwrite;
            else {
                rc = wait_event_interruptible(out->wait, ABS(out->itail-out->ihw)!=out->slots);
                if (rc < 0)
                    return rc;
            }
        }

#ifdef DEBUG
        printk("out->issues %d (%d %d %d)\n", atomic_read(&out->issues), out->ihw, out->ihead, out->itail);
#endif
        idx = regular_idx(out->itail, out->slots);
        len = (count > (bufsize-out->copys)) ? (bufsize-out->copys) : count;
        copy_from_user((void *)bus_to_virt(out->mde[idx].addr)+out->copys, (void *)p, len);
        out->copys += len;
        count -= len;
        out->info.bbytes += len;
        out->info.tbytes += len;
        p += len;
        nwrite += len;
        if (out->info.tbytes >= out->info.bytes_per_hour) {
            out->info.ptradjust++;
            out->info.tbytes -= out->info.bytes_per_hour;
        }

        if (out->copys >= out->threshold) {
            out->mde[idx].dcnt = out->copys;
            out->copys = 0;
            out->mde[idx].v = 1;    /* valid mde entry */
            out->mde[idx].e = 0;
            advance_idx(out->itail, out->slots);
            issue_out(out);
        }
    }

    return nwrite;
}


#define HW_DMA_COPY_BUG
#ifdef HW_DMA_COPY_BUG
#define is_in_empty(xin)    ((xin)->ihw  == (xin)->ihead || \
                             (xin)->ihw == ((((xin)->ihead+1) == ((xin)->slots<<1)) ? 0 : (xin)->ihead+1))
#else
#define is_in_empty(xin)    ((xin)->ihead == (xin)->ihw)
#endif

static ssize_t pl_dsp_read(struct file *file, char *buffer, size_t count, loff_t *ppos)
{
    int nread = 0;
    pl_audio_t *pl_audio = (pl_audio_t *)file->private_data;
    struct pl_audio_buf *in;
    int len, idx;
    int rc, bufsize;
    char *p;

    if (pl_audio == NULL)
        return -EINVAL;

    in = &pl_audio->in;

    if (ppos != &file->f_pos)
        return -ESPIPE;

    if (!access_ok(VERIFY_WRITE, buffer, count))
        return -EFAULT;


    in->state = ST_RW;
    bufsize = 1 << in->order;
    p = buffer;
    issue_in(in);

    while(count > 0) {
        if (is_in_empty(in)) { /* EMPTY, no more data for copying */
            if (file->f_flags & O_NONBLOCK)
                return (nread == 0) ? -EAGAIN: nread;
            else {
                rc = wait_event_interruptible(in->wait, !is_in_empty(in));
                if (rc < 0)
                    return rc;
            }
        }

#ifdef DEBUG
        printk("in->issues %d (%d %d %d)\n", atomic_read(&in->issues), in->ihead, in->ihw, in->itail);
#endif

        idx = regular_idx(in->ihead, in->slots);
        len = (count > (bufsize-in->copys)) ? (bufsize-in->copys) : count;
        pci_dma_sync_single(NULL, in->mde[idx].addr, in->mde[idx].dcnt, PCI_DMA_TODEVICE);
        copy_to_user((void *)p, (void *)bus_to_virt(in->mde[idx].addr) + in->copys, len);
        count -= len;
        nread += len;
        in->copys += len;
        in->info.bbytes -= len;
        in->info.tbytes += len;
        p += len;
        if (in->info.tbytes >= in->info.bytes_per_hour) {
            in->info.ptradjust++;
            in->info.tbytes -= in->info.bytes_per_hour;
        }

        if (in->copys >= in->threshold) {
            advance_idx(in->ihead, in->slots);
            in->copys = 0;
            issue_in(in);
        }
    }

    return nread;
}

static unsigned int pl_dsp_poll(struct file *file, poll_table *wait)
{
    pl_audio_t *pl_audio = file->private_data;
    unsigned int mask = 0;

    if (file->f_mode & FMODE_WRITE)
        poll_wait(file, &pl_audio->out.wait, wait);

    if (file->f_mode & FMODE_READ)
        poll_wait(file, &pl_audio->in.wait, wait);


    /* is not FULL? slot available for writing */
    if ((file->f_mode & FMODE_WRITE) &&
        (ABS(pl_audio->out.itail-pl_audio->out.ihw) != pl_audio->out.slots))
        mask |= (POLLOUT | POLLWRNORM);

    /* is not EMPTY? data is ready for copy */
    if ((file->f_mode & FMODE_READ) &&
        (!is_in_empty(&pl_audio->in) ||
        /* BECAUSE, driver doesn't not issue upstream DMA at device openning. */
        /* It will be triggered at first read invokation. Here must consider  */
        /* this situation else application hang of starvation */
        (pl_audio->in.ihead == pl_audio->in.itail)))
        mask |= (POLLIN | POLLRDNORM);

    return mask;
}

#define get_user_ret(x, ptr, ret) ({if (get_user(x,ptr)) return ret; })
#define validate_state(x, xstate, ret)   ({ if (x.state != xstate) return ret; })

static int pl_dsp_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    int rc = 0;
    int val = 0;
    int channels = 1;
    pl_audio_t *pl_audio = (pl_audio_t *)file->private_data;
    audio_buf_info info;
    count_info cinfo;

    switch(cmd) {
    case OSS_GETVERSION:
        rc = put_user(SOUND_VERSION, (int *)arg);
        break;
    case SNDCTL_DSP_GETFMTS:
        rc = put_user( (AFMT_U8 | AFMT_S8 | AFMT_S16_LE | AFMT_U16_LE), (int *)arg);
        break;
    case SNDCTL_DSP_SETFMT:
        get_user_ret(val, (int *)arg, -EFAULT);
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_audio->out, ST_OPEN, -EINVAL);
            val = pl_dsp_set_format(val, DIR_OUT);
            pl_calc_time_base(pl_audio, DIR_OUT);
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_audio->in, ST_OPEN, -EINVAL);
            val = pl_dsp_set_format(val, DIR_IN);
            pl_calc_time_base(pl_audio, DIR_IN);
        }
        rc = put_user(val, (int *)arg);
        break;

    case SNDCTL_DSP_CHANNELS:   /* 1 == mono, 2 == stereo */
        get_user_ret(val, (int *)arg, -EFAULT);
        if (val != 2 && val != 1)
            return -EINVAL;
        channels = val;
        /* go through to next case */
    case SNDCTL_DSP_STEREO:     /* nonzero == stereo */
        if (cmd == SNDCTL_DSP_STEREO) {
            get_user_ret(val, (int *)arg, -EFAULT);
            channels = (val) ? 2 : 1;
        }
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_audio->out, ST_OPEN, -EINVAL);
            pl_dsp_set_channels(channels, DIR_OUT);
            pl_calc_time_base(pl_audio, DIR_OUT);
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_audio->in, ST_OPEN, -EINVAL);
            pl_dsp_set_channels(channels, DIR_IN);
            pl_calc_time_base(pl_audio, DIR_IN);
        }

        rc = put_user(val, (int *)arg);
        break;

    case SNDCTL_DSP_SPEED:
        get_user_ret(val, (int *)arg, -EFAULT);
        if (val < 0)
            return -EINVAL;
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_audio->out, ST_OPEN, -EINVAL);
            val = pl_dsp_set_samplerate(&pl_audio->ac97_codec, val, DIR_OUT);
            pl_calc_time_base(pl_audio, DIR_OUT);
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_audio->in, ST_OPEN, -EINVAL);
            val = pl_dsp_set_samplerate(&pl_audio->ac97_codec, val, DIR_IN);
            pl_calc_time_base(pl_audio, DIR_IN);
        }

        rc = put_user(val, (int *)arg);
        break;

    case SNDCTL_DSP_SYNC:
        if (file->f_mode & FMODE_WRITE) {
            drain_out(&pl_audio->out);
            pl_audio->out.state = ST_OPEN;
        }
        break;

    case SNDCTL_DSP_SETDUPLEX:
        return 0;
    case SNDCTL_DSP_RESET:
        if (file->f_mode & FMODE_READ && pl_audio->in.state != ST_OPEN) {   /* 003 */
            validate_state(pl_audio->in, ST_RW, -EINVAL);
            stop_issue(&pl_audio->in);
#if AUDIO_MODULE_63
            writeb(0, PL_AUDIO_CMD_COMMAND_2);  /* force hardware to reset downstream */
#else
            writeb(COMMAND_2_DMAU, PL_AUDIO_CMD_COMMAND_2);  /* force hardware to reset downstream */
#endif
            drain_in(&pl_audio->in);
            clean_buffer(&pl_audio->in);
            pl_audio->in.state = ST_OPEN;
        }
        if (file->f_mode & FMODE_WRITE && pl_audio->out.state != ST_OPEN) { /* 003 */
            validate_state(pl_audio->out, ST_RW, -EINVAL);
            stop_issue(&pl_audio->out);
#if AUDIO_MODULE_63
            writeb(0, PL_AUDIO_CMD_COMMAND_3);  /* force hardware to reset downstream */
#else
            writeb(COMMAND_3_DMAD, PL_AUDIO_CMD_COMMAND_3);  /* force hardware to reset downstream */
#endif
            drain_out(&pl_audio->out);
            clean_buffer(&pl_audio->out);
            pl_audio->out.state = ST_OPEN;
        }
        break;
    case SNDCTL_DSP_POST:
        if (file->f_mode & FMODE_WRITE)
            pause_issue(&pl_audio->out);
        break;

    case SNDCTL_DSP_GETOSPACE:
        if (!(file->f_mode & FMODE_WRITE))
            return -EINVAL;

        if (pl_audio->out.ihw == pl_audio->out.itail)
            info.fragments = pl_audio->out.slots;
        else {
            info.fragments = regular_idx(pl_audio->out.ihw, pl_audio->out.slots) -
                             regular_idx( pl_audio->out.itail, pl_audio->out.slots);
            if (info.fragments < 0)
                info.fragments = pl_audio->out.slots + info.fragments;
        }

        info.fragstotal = pl_audio->out.slots;
        info.fragsize = 1 << pl_audio->out.order;
        info.bytes = info.fragments * info.fragsize;
        rc = copy_to_user((void *)arg, &info, sizeof(audio_buf_info));
        break;

    case SNDCTL_DSP_GETISPACE:
        if (!(file->f_mode & FMODE_READ))
            return -EINVAL;

        if (pl_audio->in.ihw == pl_audio->in.ihead)
            info.fragments = 0;
        else {
            info.fragments = regular_idx(pl_audio->in.ihw, pl_audio->in.slots) -
                             regular_idx( pl_audio->in.ihead, pl_audio->in.slots);
            if (info.fragments <= 0)
                info.fragments = pl_audio->in.slots + info.fragments;
        }

        info.fragstotal = pl_audio->in.slots;
        info.fragsize = 1 << pl_audio->in.order;
        info.bytes = info.fragments * info.fragsize;
        rc = copy_to_user((void *)arg, &info, sizeof(audio_buf_info));
        break;

    case SNDCTL_DSP_SETFRAGMENT:
        get_user_ret(val, (int *)arg, -EFAULT);
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_audio->out, ST_OPEN, -EINVAL);
            rc = pl_dsp_setfragment(&pl_audio->out, 0, ((unsigned int)val) >> 16, val & 0xffff );
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_audio->in, ST_OPEN, -EINVAL);
            rc = pl_dsp_setfragment(&pl_audio->in, 1, ((unsigned int)val) >> 16, val & 0xffff );
        }
        break;
    case SNDCTL_DSP_GETOPTR:
        if (!(file->f_mode & FMODE_WRITE))
            return -EINVAL;
        pl_dsp_getoptr(&pl_audio->out, &cinfo);
        rc = copy_to_user((void *)arg, &cinfo, sizeof(count_info));
        break;
    case SNDCTL_DSP_GETIPTR:
        if (!(file->f_mode & FMODE_READ))
            return -EINVAL;
        pl_dsp_getiptr(&pl_audio->in, &cinfo);
        rc = copy_to_user((void *)arg, &cinfo, sizeof(count_info));
        break;
    case SNDCTL_DSP_NONBLOCK:
        file->f_flags |= O_NONBLOCK;
        return 0;

    case SNDCTL_DSP_GETCAPS:
        rc = put_user(DSP_CAP_DUPLEX | DSP_CAP_REALTIME | DSP_CAP_TRIGGER, (int *) arg);
        break;
    case SNDCTL_DSP_SETTRIGGER:
        get_user_ret(val, (int *)arg, -EFAULT);
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_audio->out, ST_OPEN, -EINVAL);

            if (val & PCM_ENABLE_OUTPUT)
                start_issue(&pl_audio->out);
            else
                stop_issue(&pl_audio->out);

        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_audio->in, ST_OPEN, -EINVAL);

            if (val & PCM_ENABLE_INPUT)
                start_issue(&pl_audio->in);
            else
                stop_issue(&pl_audio->in);
        }
        break;
    case SNDCTL_DSP_GETTRIGGER:
        if (file->f_mode & FMODE_WRITE) {
            if (pl_audio->out.start)
                val |= PCM_ENABLE_OUTPUT;
        }
        if (file->f_mode & FMODE_READ) {
            if (pl_audio->in.start)
                val |= PCM_ENABLE_INPUT;
        }
        break;

    case SNDCTL_DSP_GETBLKSIZE:
        if (file->f_mode & FMODE_WRITE)
            val = 1 << pl_audio->out.order;
        else if (file->f_mode & FMODE_READ)
            val = 1 << pl_audio->in.order;
        rc = put_user(val, (int*)arg);
        break;

    case SOUND_PCM_READ_RATE:
        if (file->f_mode & FMODE_WRITE)
            val = pl_dsp_get_samplerate(&pl_audio->ac97_codec, DIR_OUT);
        else if (file->f_mode & FMODE_READ)
            val = pl_dsp_get_samplerate(&pl_audio->ac97_codec, DIR_IN);
        rc = put_user(val, (int *)arg);
        break;

    case SOUND_PCM_READ_CHANNELS:
        if (file->f_mode & FMODE_WRITE)
            val = pl_dsp_get_channels(DIR_OUT);
        else if (file->f_mode & FMODE_READ)
            val = pl_dsp_get_channels(DIR_IN);

        rc = put_user(val, (int *)arg);
        break;

    case SOUND_PCM_READ_BITS:
        if (file->f_mode & FMODE_WRITE)
            val = pl_dsp_get_format(DIR_OUT);
        else if (file->f_mode & FMODE_READ)
            val = pl_dsp_get_format(DIR_IN);

        rc = put_user(val, (int *)arg);
        break;

    case SNDCTL_DSP_GETODELAY:
        if (file->f_mode & FMODE_WRITE)
            val = pl_dsp_getodelay(&pl_audio->out);
        else
            val = 0;

        rc = put_user(val, (int *)arg);
        break;
    /* Prolific speical audio ioctl options */
    case SNDCTL_DSP_GETIDELAY:
        if (file->f_mode & FMODE_WRITE)
            val = pl_dsp_getidelay(&pl_audio->in);
        else
            val = 0;

        rc = put_user(val, (int *)arg);
        break;

    case PL_SNDCTL_DSP_SYMALR_REQ :
        if (file->f_mode & FMODE_WRITE) {
            PLSymbolAlarm SymbolAlarm;
            int ord;

            validate_state(pl_audio->out, ST_OPEN, -EINVAL);
            rc = copy_from_user(&SymbolAlarm, (void *)arg, sizeof(PLSymbolAlarm));
            if (rc < 0)
                return -EFAULT;
            if (SymbolAlarm.nPID == 0)
                SymbolAlarm.nPID = current->pid;
            if (SymbolAlarm.nMaxBufferSize > 0 && SymbolAlarm.nMaxBufferAmount > 0) {
                ord = get_order_by_size(SymbolAlarm.nMaxBufferSize);
                rc = pl_dsp_setfragment(&pl_audio->out, 0, SymbolAlarm.nMaxBufferAmount, ord);
                if (rc < 0)
                     break;
            }
            SymbolAlarm.nMaxBufferAmount = pl_audio->out.slots;
            SymbolAlarm.nMaxBufferSize = 1 << pl_audio->out.order;
            pl_audio->out.threshold = (SymbolAlarm.nFrequency > SymbolAlarm.nMaxBufferSize) ?
                                    SymbolAlarm.nMaxBufferSize : SymbolAlarm.nFrequency;
            pl_audio->out.alarmpid = SymbolAlarm.nPID;
            rc = copy_to_user((void *)arg, &SymbolAlarm, sizeof(PLSymbolAlarm));
        } else
            return -EINVAL;
        break;

    case PL_SNDCTL_DSP_SYMALR_FREQUENCY:
        if (file->f_mode & FMODE_WRITE) {
            get_user_ret(val, (int *)arg, -EFAULT);
            if (val > (1 << pl_audio->out.order) || val < 0)
                return -EINVAL;
            pl_audio->out.threshold = val;
            if (pl_audio->out.alarmpid == 0)
                pl_audio->out.alarmpid = current->pid;
        } else
            return -EINVAL;
         break;
    case PL_SNDCTL_DSP_SYMALR_ACTIVE :
        if (file->f_mode & FMODE_WRITE) {
            get_user_ret(val, (int *)arg, -EFAULT);
            switch(val) {
            case PL_SYMBOL_ALARM_QUERY:
                val = (pl_audio->out.alarm) ? PL_SYMBOL_ALARM_ENABLE : PL_SYMBOL_ALARM_DISABLE;
                rc = put_user(val, (int *)arg);
                break;

            case PL_SYMBOL_ALARM_DISABLE:
                pl_audio->out.alarm = 0;
                break;
            case PL_SYMBOL_ALARM_ENABLE:
                pl_audio->out.alarm = 1;
                break;
            default:
                rc = -EINVAL;
            }
        } else
            return -EINVAL;
        break;

    case PL_SNDCTL_DSP_SYMALR_REL :
        if (file->f_mode & FMODE_WRITE) {
            pl_audio->out.threshold = (1 << pl_audio->out.order);
            pl_audio->out.alarm = 0;
        } else
            return -EINVAL;
        break;

    default:
        printk("unhandled ioctl, cmd=%u arg=%ld\n", cmd, arg);
        rc = -EINVAL;
        break;
    }
    return rc;
}



static int pl_dsp_mmap(struct file *file, struct vm_area_struct *pvma)
{
    return -EINVAL;
}


static struct file_operations pl_dsp_ops =
{
    owner:      THIS_MODULE,
    open:       pl_dsp_open,
    release:    pl_dsp_release,
    read:       pl_dsp_read,
    write:      pl_dsp_write,
    ioctl:      pl_dsp_ioctl,
    poll:       pl_dsp_poll,
    llseek:     no_llseek,
    mmap:       pl_dsp_mmap,
};



/******************************************************************************
 *  Mixer functions
 ******************************************************************************/
static int pl_mixer_open(struct inode *inode, struct file *file)
{
    int minor = MINOR(inode->i_rdev);
    if (pl_audio_ins.ac97_codec.dev_mixer != minor)
        return -ENODEV;

    file->private_data = &pl_audio_ins.ac97_codec;
    return 0;
}


static int pl_mixer_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    struct ac97_codec *codec = file->private_data;

    return codec->mixer_ioctl(codec, cmd, arg);
}

static int pl_mixer_release(struct inode *inode, struct file *file)
{
    return 0;
}


static struct file_operations pl_mixer_ops =
{
    owner:      THIS_MODULE,
    open:       pl_mixer_open,
    llseek:     no_llseek,
    ioctl:      pl_mixer_ioctl,
    release:    pl_mixer_release,
};



/******************************************************************************
 *  audio proc information
 ******************************************************************************/

static int pl_audio_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    pl_audio_t *pl_audio = data;
    struct pl_audio_info *out_info = &pl_audio->out.info;
    struct pl_audio_info *in_info = &pl_audio->in.info;
    len += sprintf(page+len, "Prolific ac97 driver info\n");
#if AUDIO_MODULE_63
    len += sprintf(page+len, "63 Audio Module\n");
#else
    len += sprintf(page+len, "61 Audio Module\n");
#endif
    len += sprintf(page+len, "Downstream:\n");
    len += sprintf(page+len, "  fragments: %d\n", out_info->slots);
    len += sprintf(page+len, "  underrun:  %d\n", out_info->runs);
    len += sprintf(page+len, "  jam:       %d\n", out_info->jam);
    len += sprintf(page+len, "  losts:     %d\n", out_info->losts);
    len += sprintf(page+len, "  restores:  %d\n", out_info->restores);
    len += sprintf(page+len, "  flags:     %s%s%s%s\n",
        (out_info->flags & HW_FLAG_EMP) ? "EMP " : "",
        (out_info->flags & HW_FLAG_FUL) ? "FUL " : "",
        (out_info->flags & HW_FLAG_OVF) ? "OVF " : "",
        (out_info->flags & HW_FLAG_UNF) ? "UNF " : "");
    len += sprintf(page+len, "Upstream:\n");
    len += sprintf(page+len, "  fragments: %d\n", in_info->slots);
    len += sprintf(page+len, "  overrun:   %d\n", in_info->runs);
    len += sprintf(page+len, "  jam:       %d\n", in_info->jam);
    len += sprintf(page+len, "  losts:     %d\n", in_info->losts);
    len += sprintf(page+len, "  restores:  %d\n", in_info->restores);
    len += sprintf(page+len, "  flags:     %s%s%s%s\n",
        (in_info->flags & HW_FLAG_EMP) ? "EMP " : "",
        (in_info->flags & HW_FLAG_FUL) ? "FUL " : "",
        (in_info->flags & HW_FLAG_OVF) ? "OVF " : "",
        (in_info->flags & HW_FLAG_UNF) ? "UNF " : "");


    return len;
}


/******************************************************************************
 *  Register and unregister device functions
 ******************************************************************************/

static void __init pl_audio_ac97_hw_reset(void)
{
    /* reset ac97 function register */
    writel(0, PL_AUDIO_CMD_FUNCTION_0);
    writeb(FUNCTION_1_VRA, PL_AUDIO_CMD_FUNCTION_1);

    /* set internal clock */
    writel(readl(PL_AUDIO_CMD_AC97CLOCK) & ~AC97CLOCK_CLKE, PL_AUDIO_CMD_AC97CLOCK);

    /* program GPIO pins to all output*/
    writeb(GPIO_ALL_PIN, PL_AUDIO_CMD_GPIO_C_ENB);  /* select pin as GPIO function */
    writeb(GPIO_ALL_PIN, PL_AUDIO_CMD_GPIO_C_OE);   /* all pin configuare as GPIO output */
    writeb(0, PL_AUDIO_CMD_GPIO_C_DO);              /* reset ac97 codec by low RESET# */
    mdelay(1);
    writeb(GPIO_AC97_RSTN, PL_AUDIO_CMD_GPIO_C_DO); /* release ac97 codec by low RESET# */


    /* program GPIO pins as AC97 functions */
    /* BIT_CLOCK, SI to input and RSTN, SYNC, SO to output */
    // writeb(GPIO_AC97_RSTN | GPIO_AC97_SYNC | GPIO_AC97_SO, PL_AUDIO_CMD_GPIO_C_OE);
    writeb(GPIO_AC97_SYNC | GPIO_AC97_SO, PL_AUDIO_CMD_GPIO_C_OE);
    writeb(0, PL_AUDIO_CMD_GPIO_C_DO);              /* reset ac97 codec by low RESET# */
    writeb(0, PL_AUDIO_CMD_GPIO_C_ENB);             /* all pin configuare as AC97 function */
    mdelay(1);

#if 1   /* A_RSTN may not function, replace by GPIO's RESET# */
    /* reset ac97 function register */
    writel(0, PL_AUDIO_CMD_FUNCTION_0);

    /* enable ac97 codec */
    writeb(FUNCTION_1_A_RSTN | FUNCTION_1_VRA , PL_AUDIO_CMD_FUNCTION_1);   /* reset ac97 codec */
    mdelay(1);
#endif

    /* set external bit clock */
    writel(readl(PL_AUDIO_CMD_AC97CLOCK) | AC97CLOCK_CLKE, PL_AUDIO_CMD_AC97CLOCK);
}


static void __init pl_audio_init_proc(void)
{
    struct proc_dir_entry *audio_proc;

    audio_proc = proc_mkdir("audio", 0);
    if (audio_proc == NULL)
        goto EXIT;

    audio_proc->owner = THIS_MODULE;
    create_proc_read_entry("ac97", 0, audio_proc, ac97_read_proc, &pl_audio_ins.ac97_codec);
    create_proc_read_entry("info", 0, audio_proc, pl_audio_read_proc, &pl_audio_ins);

EXIT:
    return;
}

static int __init pl_audio_init(void)
{
    int rc;

#if AUDIO_MODULE_63
    printk("Prolific Audio AC97 driver version %s for 63 and 29 Audio Module 2006/03/21\n", VERSION);
#else
    printk("Prolific Audio AC97 driver version 2.0.0 2004/07/20\n");
#endif
    memset(&pl_audio_ins, 0, sizeof(pl_audio_t));

    /* set ac97 clock divider */
    if (pl_get_dev_hz() == 96000000) {
        writeb(6, PL_CLK_AC97);                 /* target 24MHz */
    } else if (pl_get_dev_hz() == 32000000) {
        writeb(4, PL_CLK_AC97);                 /* target 32HHz */
    } else if (pl_get_dev_hz() == 120000000) {
        writeb(6, PL_CLK_AC97);                 /* target 30MHz */
    } else {
        printk("Unsupported device clock %d Hz\n", pl_get_dev_hz());
        return -EINVAL;
    }


    /* ac97 codec reset */
    pl_audio_ac97_hw_reset();

    /* ac97 controller reset */
    writeb(0, PL_AUDIO_CMD_COMMAND_1);
    writeb(COMMAND_1_B_RSTN, PL_AUDIO_CMD_COMMAND_1);

    /* set ac97 mode */
    writeb(COMMAND_0_AC97, PL_AUDIO_CMD_COMMAND_0);

    /* set ac97 output TAG */
    writew(0x9800, PL_AUDIO_CMD_AC97CONFIG_0);

    /*
     * Register a dsp device
     */
    /* init ac97 codec tool functions */
    pl_audio_ins.ac97_codec.private_data = &pl_audio_ins;
    pl_audio_ins.ac97_codec.codec_read = pl_codec_read;
    pl_audio_ins.ac97_codec.codec_write = pl_codec_write;
    pl_audio_ins.ac97_codec.codec_wait = pl_codec_wait;

    /* probe ac97 codec */
    if (ac97_probe_codec(&pl_audio_ins.ac97_codec) == 0) {
        printk("Unable to probe AC97 codec, aborting\n");
        rc = -ENODEV;
        goto EXIT;
    }

    pl_audio_ins.ac97_codec.dev_mixer = register_sound_mixer(&pl_mixer_ops, -1);
    if (pl_audio_ins.ac97_codec.dev_mixer < 0) {
        printk("unable to register AC97 mixer, aborting\n");
        rc = pl_audio_ins.ac97_codec.dev_mixer;
        goto EXIT;
    }

    pl_audio_ins.dspo = register_sound_dsp(&pl_dsp_ops, -1);
    if (pl_audio_ins.dspo < 0) {
        printk("unable to register dsp output device, aborting\n");
        rc = pl_audio_ins.dspo;
        goto EXIT;
    }

    pl_audio_ins.dspi = register_sound_dsp(&pl_dsp_ops, -1);
    if (pl_audio_ins.dspi < 0) {
        printk("unable to register dsp output device, aborting\n");
        rc = pl_audio_ins.dspi;
        goto EXIT;
    }

    pl_codec_write(&pl_audio_ins.ac97_codec, AC97_EXTENDED_STATUS, 1); /* enable ac97 VRA variable rate */

    /* initial out/in buffer */
    pl_dsp_setfragment(&pl_audio_ins.out, 0, DEFAULT_OUT_BUF_SLOTS, DEFAULT_OUT_BUF_SLOT_SIZE_ORDER);
    init_waitqueue_head(&pl_audio_ins.out.wait);
    init_waitqueue_head(&pl_audio_ins.out.drain);
    atomic_set(&pl_audio_ins.out.issues, 0);
    pl_audio_ins.out.issue = issue_out;

    pl_dsp_setfragment(&pl_audio_ins.in, 1, DEFAULT_IN_BUF_SLOTS, DEFAULT_IN_BUF_SLOT_SIZE_ORDER);
    init_waitqueue_head(&pl_audio_ins.in.wait);
    init_waitqueue_head(&pl_audio_ins.in.drain);
    atomic_set(&pl_audio_ins.in.issues, 0);
    pl_audio_ins.in.issue = issue_in;


#ifdef  CONFIG_PL_AC97_POWERDOWN
    /* 2004/08/06 pax: power down all */
    pl_dsp_power_down(&pl_audio_ins, 1);
    pl_dsp_power_down(&pl_audio_ins, 0);
#endif

    /* init dma channel */
    rc = request_dma(DMA_PL_AC97_OUT, "AC97 DS");
    if (rc < 0)
        goto EXIT;

    set_dma_mde_base(DMA_PL_AC97_OUT, (const char *)pl_audio_ins.out.mde);
    set_dma_mode(DMA_PL_AC97_OUT, DMA_MODE_WRITE);
#if AUDIO_MODULE_63_DMA_LINE_BUG
    set_dma_burst_length(DMA_PL_AC97_OUT, 0);
#else
    set_dma_burst_length(DMA_PL_AC97_OUT, DMA_BURST_LINE);
#endif

    rc = request_dma(DMA_PL_AC97_IN, "AC97 US");
    if (rc < 0)
        goto EXIT;

    set_dma_mde_base(DMA_PL_AC97_IN, (const char *)pl_audio_ins.in.mde);
    set_dma_mode(DMA_PL_AC97_IN, DMA_MODE_READ);
#if AUDIO_MODULE_63_DMA_LINE_BUG
    set_dma_burst_length(DMA_PL_AC97_IN, 0);
#else
    set_dma_burst_length(DMA_PL_AC97_IN, DMA_BURST_LINE);
#endif

    /* init irq handler */
    rc = request_irq(IRQ_PL_AC970, pl_audio_handler, 0, "Audio AC97", &pl_audio_ins);
    if (rc < 0)
        goto EXIT;

    /* ac97 controller enable interrupt */
#if AUDIO_MODULE_63
    writeb(COMMAND_1_B_RSTN | COMMAND_1_INTE_DNDA | COMMAND_1_INTE_UPDA, PL_AUDIO_CMD_COMMAND_1);
#else
    writeb(COMMAND_0_AC97 | COMMAND_0_INTE, PL_AUDIO_CMD_COMMAND_0);
#endif

    /* create proc entry */
    pl_audio_init_proc();

    return 0;
EXIT:
    if (pl_audio_ins.dspi > 0)
        unregister_sound_dsp(pl_audio_ins.dspi);

    if (pl_audio_ins.dspo > 0)
        unregister_sound_dsp(pl_audio_ins.dspo);

    if (pl_audio_ins.ac97_codec.dev_mixer > 0)
        unregister_sound_dsp(pl_audio_ins.ac97_codec.dev_mixer);

    return rc;
}

module_init(pl_audio_init);

#ifdef CONFIG_MODULES
static void __exit pl_audio_clean(void)
{
    printk(KERN_INFO "pl_audio: unloading\n");

}
module_exit(pl_audio_clean);
#endif

/**
 * Grammar:
 *    pl_audio=[outbuf(size_order,number)][,inbuf(size_order,number)]
 *    the size is a fragment order size, it can be 256, 512, 1024, 2048, 4096(defult), 8192 and 16384
 *    the number is a maxiumn fragments, it can be 4 ~ 64
 */
static int __init pl_audio_setup(char *options)
{
    int size;
    if (strncmp(options, "inbuf(",6) == 0) {
        options += 6;
        size = simple_strtoul(options, &options, 0);
        DEFAULT_OUT_BUF_SLOT_SIZE_ORDER = get_order_by_size(size);
        if (*options != ',')
            goto EXIT;
        options++;
        DEFAULT_OUT_BUF_SLOTS = simple_strtoul(options, &options, 0);
        if (*options != ')')
            goto EXIT;
        options++;

        if (*options == ',')
            options++;
    }

    if (strncmp(options, "outbuf(", 7) == 0) {
        options += 7;
        size = simple_strtoul(options, &options, 0);
        DEFAULT_IN_BUF_SLOT_SIZE_ORDER = get_order_by_size(size);
        if (*options != ',')
            goto EXIT;
        options++;
        DEFAULT_IN_BUF_SLOTS = simple_strtoul(options, &options, 0);
        if (*options != ')')
            goto EXIT;
        options++;
    }
EXIT:

    if (DEFAULT_OUT_BUF_SLOT_SIZE_ORDER < 8)
        DEFAULT_OUT_BUF_SLOT_SIZE_ORDER = 8;
    else if (DEFAULT_OUT_BUF_SLOT_SIZE_ORDER > 15)
        DEFAULT_OUT_BUF_SLOT_SIZE_ORDER = 15;

    if (DEFAULT_IN_BUF_SLOT_SIZE_ORDER < 8)
        DEFAULT_IN_BUF_SLOT_SIZE_ORDER = 8;
    else if (DEFAULT_IN_BUF_SLOT_SIZE_ORDER > 15)
        DEFAULT_IN_BUF_SLOT_SIZE_ORDER = 15;

    if (DEFAULT_OUT_BUF_SLOTS > 64)
        DEFAULT_OUT_BUF_SLOTS = 64;
    if (DEFAULT_IN_BUF_SLOTS > 64)
        DEFAULT_IN_BUF_SLOTS = 64;

    if (DEFAULT_OUT_BUF_SLOTS <= 0)
        DEFAULT_OUT_BUF_SLOTS = 4;

    if (DEFAULT_IN_BUF_SLOTS <= 0)
        DEFAULT_IN_BUF_SLOTS = 4;


    return 1;
}

__setup("pl_audio=", pl_audio_setup);


MODULE_AUTHOR("Jedy Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("PL1061 ac97 audio driver");

