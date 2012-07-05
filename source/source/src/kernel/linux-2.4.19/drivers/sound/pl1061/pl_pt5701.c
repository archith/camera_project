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
#include <asm/pl1061/dma.h>
#include <asm/pl1061/irq.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/major.h>
#include <linux/devfs_fs_kernel.h>
#include <asm/arch-pl1029/pl_symbol_alarm.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/pm.h>

#ifdef CONFIG_PL1063
#define AUDIO_MODULE_63     1
#define AUDIO_MODULE_63_DMA_LINE_BUG            1
#define AUDIO_MODULE_63_LINE_RESIDUE_BUG        1
#define AUDIO_MODULE_TRIM_DSP_WRITE_TO_DWORD    1
#else
#define AUDIO_MODULE_61     1
#endif


/******************************************************************************
 * PT5701 proive USB mode and Normal mode conrtol, this driver support
 * USB mode and 12MHz MCLK only.
 ******************************************************************************/


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

#define PL_AUDIO_CMD_I2S_DOWN       (PL_AUDIO_CMD + 0x30)
#define PL_AUDIO_CMD_I2S_UP         (PL_AUDIO_CMD + 0x34)

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
#define I2SCONFIG_3_AUSM_MASK       (0x3 << 6)  /* up stream stereo/mono */
#define I2SCONFIG_3_AUSM_STEREO     (0x0 << 6)  /* 0x stereo */
#define I2SCONFIG_3_AUSM_MONO_LEFT  (0x2 << 6)  /* 10 mono left */
#define I2SCONFIG_3_AUSM_MONO_RIGHT (0x3 << 6)  /* 11 mono right */

#define I2SCONFIG_3_AUPS_MASK       (0x3 << 4)  /* up stream channel precision */
#define I2SCONFIG_3_AUPS_20         (0x0 << 4)  /* 00: 20bits/channel */
#define I2SCONFIG_3_AUPS_18         (0x1 << 4)  /* 01: 18bits/channel */
#define I2SCONFIG_3_AUPS_16         (0x2 << 4)  /* 10: 16bits/channel */
#define I2SCONFIG_3_AUPS_8          (0x3 << 4)  /* 11: 8bits/channel */

#define I2SCONFIG_2_ADSM_MASK       (0x3 << 6)  /* down stream stereo/mono */
#define I2SCONFIG_2_ADSM_STEREO     (0x0 << 6)  /* 00 normal (stereo) */
#define I2SCONFIG_2_ADSM_MONO       (0x1 << 6)  /* 01 mono right (slot4) */
#define I2SCONFIG_2_ADSM_MONO_RIGHT (0x1 << 6)  /* 01 mono right (slot4) */
#define I2SCONFIG_2_ADSM_MONO_LEFT  (0x2 << 6)  /* 10 mono left (slot3) */


#define I2SCONFIG_2_ADPS_MASK       (0x3 << 4)  /* down stream channel precision */
#define I2SCONFIG_2_ADPS_20         (0x0 << 4)  /* 00: 20bits/channel */
#define I2SCONFIG_2_ADPS_18         (0x1 << 4)  /* 01: 18bits/channel */
#define I2SCONFIG_2_ADPS_16         (0x2 << 4)  /* 10: 16bits/channel */
#define I2SCONFIG_2_ADPS_8          (0x3 << 4)  /* 11: 8bits/channel */


/* I2S Down Stream and Up Stream Channel */
#if AUDIO_MODULE_63
#define I2SDOWN_DLENG_OFF           8
#define I2SDOWN_DOFFS_OFF           20
#define I2SUP_ULENG_OFF             8
#define I2SUP_UOFFS_OFF             20
#else
#define I2SDOWN_DLENG_OFF           16
#define I2SDOWN_DOFFS_OFF           0
#define I2SUP_ULENG_OFF             16
#define I2SUP_UOFFS_OFF             0
#endif


#if AUDIO_MODULE_63
#define I2S_WSO_MODE                (0x00 << 4)
#define I2S_WSI_MODE                (0x00 << 0)
#else
#define I2S_WSO_MODE                (0x00 << 4)
#define I2S_WSI_MODE                (0x02 << 0)
#endif


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


/******************************************************************************
 * Prolific PT5701 Regsiter Definition
 ******************************************************************************/
#define PT_PGA                      0x00        /* Input volume control */

#define PT_LOUT_VOL                 0x02        /* Left Line out volume */
#define PT_ROUT_VOL                 0x03        /* Right Line out volume */

#define PT_DAC_MUTE                 0x05        /* Digital Soft mute */

#define PT_AUDIO_INF                0x07        /* Digital Audio Infterface */
#define PT_SAMPLE_RATE              0x08        /* Clocking and Sample Rate control */

#define PT_DAC_L_VOL                0x0a        /* Left DAC Digital Volume */
#define PT_DAC_R_VOL                0x0b        /* Right DAC Digital Volume */

#define PT_RESET                    0x0f        /* Reset */

#define PT_ADC_L_VOL                0x15        /* Left ADC Digital Volume */
#define PT_ADC_R_VOL                0x16        /* Right ADC Digital Volume */

#define PT_CONTROL                  0x18
#define PT_POWER_1                  0x19        /* power management (1) */
#define PT_POWER_2                  0x1a        /* power management (2) */

#define PT_ADC_R                    0x21        /* ADC signal path control left */
#define PT_ADC_L                    0x20        /* ADC signal path control right */

#define PT_PW_VMIDSEL   (1 << 7)        /* Output BIAS enable */
#define PT_PW_AINL      (1 << 5)        /* Micboost enable */
#define PT_PW_ADCL      (1 << 3)        /* PGA and SAR ADC enable */
#define PT_PW_MICB      (1 << 1)        /* Microphone bias enable */
#define PT_PW_DACL      (1 << 8)        /* DS DAC enable */

#define PT_MUTE         (1 << 3)        /* mute */


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

struct pl_i2s_info {
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

struct pl_i2s_buf {
    int         state;                  /* ST_CLOSE, ST_OPEN, ST_RW */
    int         suspend;                /* 1 Device suspend now */
    int         start;
    int         slots;
    int         order;                  /* the buffer size order, the buffer size = 1 << order */
    int         ihw;                    /* index to hardware current function slots */
    int         ihead;                  /* index to next ready mde slot */
    int         itail;                  /* index to next available (free) mde slot */
    atomic_t    issues;
    int         copys;                  /* the current slots copy from/to user buffer */
    int         threshold;              /* a slot should be filled at least size to send to device */
    struct dma_mde  mde[MAX_SLOTS] __attribute__((aligned (8)));    /* #002 */
    int         hw_error;               /* once hardware underflow, driver should wait DMA stopping */
    wait_queue_head_t   wait;           /* wait for buffer ready (read) or available (write) */
    wait_queue_head_t   drain;          /* wait for buffer drain out */
    wait_queue_head_t   apm_wait;       /* wait for apm resume */
    int  (*issue)(struct pl_i2s_buf *abuf);
    struct  pl_i2s_info info;
    int         alarmpid;               /* pid for alarm */
    int         alarm;                  /* alarm active flag */
};


typedef struct pl_i2s_tag {
    int                 dspo;           /* dsp output device */
    int                 oused;
    int                 osrate;         /* output sample rate */
    struct pl_i2s_buf   out;
    int                 dspi;           /* dsp intput device */
    int                 iused;
    int                 israte;         /* input sample rate */
    struct pl_i2s_buf   in;
    /* mixer device */
    int                 dev_mixer;
    int                 micboost;
    int                 recio;          /* 0 line in, 1 mic */
    int                 modcnt;
    int                 power;
    unsigned int        mixer_state[3];
} pl_i2s_t;

static pl_i2s_t pl_i2s_ins;


static int pt5701_write(int reg, int value);
static int pt5701_read(int reg, int *value);

#define CONFIG_DE_SOUND_BURST

static int pt5701_dac_left = 0x70;
static int pt5701_dac_right = 0x70;

#ifdef CONFIG_DE_SOUND_BURST

static int dsb_start_val[] = {
    0xe3, 0xca, 0xb4, 0xa0, 0x8e,
    0x7f, 0x71, 0x64, 0x59, 0x4f,
    0x18, 0x06, 0x00
};


void de_sound_burst_start(void)
{
    int i;
    for (i = 0; i < sizeof(dsb_start_val)/sizeof(&dsb_start_val[0]); i++) {
        pt5701_write(0x0a,dsb_start_val[i]);
        pt5701_write(0x0b,dsb_start_val[i]);
    }
}

static int dsb_play_stop[] = {
    0x64, 0x59, 0x4f, 0x18, 0x06, 0x00
};

void de_sound_burst_play_start(void)
{
    pt5701_write(0x0a, pt5701_dac_left);
    pt5701_write(0x0b, pt5701_dac_right);
}


void de_sound_burst_play_stop(void)
{
    int i;
    for (i = 0; i < sizeof(dsb_play_stop)/sizeof(&dsb_play_stop[0]); i++) {
        pt5701_write(0x0a,dsb_play_stop[i]);
        pt5701_write(0x0b,dsb_play_stop[i]);
    }

}

#else

#define de_sound_burst_start()          {}
#define de_sound_burst_play_start()     {}
#define de_sound_burst_play_stop()      {}

#endif /* CONFIG_DE_SOUND_BURST */




/******************************************************************************
 *  APM
 ******************************************************************************/
#ifdef CONFIG_APM
static struct pm_dev *pldsp_pm_dev = NULL;
#endif /* CONFIG_APM */



/******************************************************************************
 *  downstream and upstream buffer mantain functions
 ******************************************************************************/
#define xDEBUG
/**
 *      ihw ihead              itail
 *       v   v                   v
 *  +-------------------------------------+
 *  | | |*|*|.|.|.|.|.|.|.|.|.|.| | | | | |
 *  +-------------------------------------+
 *    issueing  ready but not issue
 */
static spinlock_t  issue_lock = SPIN_LOCK_UNLOCKED;
static int issue_out(struct pl_i2s_buf *out)
{
    int flags = 0;
    int idx;
    // int l;
    struct pl_i2s_info *out_info = &out->info;

    if (get_dma_residue(DMA_PL_AC97_OUT) == 0) {
        if (out->hw_error) {
            out->hw_error = 0;
            out_info->restores++;
        }
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

    /* it should be checked agian before issuing */
    if (out->ihead != out->itail &&
        !out->hw_error && atomic_read(&out->issues) < 2) {
        idx = regular_idx(out->ihead, out->slots);

#if AUDIO_MODULE_63_LINE_RESIDUE_BUG
        if (out->mde[idx].dcnt < 16)
            out->mde[idx].dcnt = 16;
#endif

#if 0
        l = out->mde[idx].dcnt >> 2;

        if (out->mde[idx].dcnt & 15) {
            out->mde[idx].dcnt = (out->mde[idx].dcnt + 15) & (-16);
        }

printk("issue %08x (%08x)\n", l, out->mde[idx].dcnt);

#endif

        pci_map_single(NULL, bus_to_virt(out->mde[idx].addr), out->mde[idx].dcnt, PCI_DMA_TODEVICE);
        pci_map_single(NULL, out->mde, MAX_SLOTS * sizeof(struct dma_mde), PCI_DMA_TODEVICE);
#if AUDIO_MODULE_63
        writew(out->mde[idx].dcnt >> 2, PL_AUDIO_CMD_DOWNPACK);
        // writew(l, PL_AUDIO_CMD_DOWNPACK);
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


static int issue_in(struct pl_i2s_buf *in)
{
    int flags = 0;
    int idx;
    int i;
    struct pl_i2s_info *in_info = &in->info;

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


static int drain_out(struct pl_i2s_buf *out)
{
    // printk("drain_out %d\n", get_dma_residue(DMA_PL_AC97_OUT));
    int idx;

    de_sound_burst_play_stop();
    pt5701_write(PT_DAC_MUTE, PT_MUTE); /* MUTE PT5701 */

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

    wait_event(out->drain, get_dma_residue(DMA_PL_AC97_OUT) == 0);

#if 0
    if (get_dma_residue(DMA_PL_AC97_OUT) != 0)
        sleep_on_timeout(&out->drain, 10*HZ);
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

static int drain_in(struct pl_i2s_buf *in)
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

static void inline start_issue(struct pl_i2s_buf *abuf)
{
    abuf->start = 1;
    if (abuf->state == ST_RW)
        abuf->issue(abuf);
    if (&pl_i2s_ins.out == abuf) {
        pt5701_write(PT_DAC_MUTE, 0);
        pt5701_write(PT_POWER_2, PT_PW_DACL);
        de_sound_burst_play_start();
    }

#ifdef CONFIG_APM
    pm_access(pldsp_pm_dev);
#endif
}

static void inline stop_issue(struct pl_i2s_buf *abuf)
{
    abuf->start = 0;
    if (&pl_i2s_ins.out == abuf) {
        de_sound_burst_play_stop();
        pt5701_write(PT_DAC_MUTE, PT_MUTE);
    }

#ifdef CONFIG_APM
    if (pl_i2s_ins.in.start == 0 && pl_i2s_ins.out.start == 0)
        pm_dev_idle(pldsp_pm_dev);
#endif

}

static void pause_issue(struct pl_i2s_buf *abuf)
{
    if (abuf->start)
        stop_issue(abuf);
    else
        start_issue(abuf);
}

static void inline clean_buffer(struct pl_i2s_buf *abuf)
{
    abuf->ihw = abuf->itail = abuf->ihead = 0;
    atomic_set(&abuf->issues, 0);
    abuf->copys = 0;
    abuf->hw_error = 0;
}

/******************************************************************************
 *  interrupt handler
 ******************************************************************************/
static void pl_i2s_handler(int irq, void *dev_id, struct pt_regs* regs)
{
    pl_i2s_t *pl_i2s = dev_id;
    struct pl_i2s_buf *out = &pl_i2s->out;
    struct pl_i2s_buf *in = &pl_i2s->in;
    struct pl_i2s_info *out_info = &out->info;
    struct pl_i2s_info *in_info = &in->info;
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
            out->mde[regular_idx(out->ihw,out->slots)].v = 0;   /* invalid mde entry */
            out_info->bbytes -= out->mde[regular_idx(out->ihw, out->slots)].dcnt;
            advance_idx(out->ihw, out->slots);
        }
    }

    if (atomic_read(&out->issues) == 1) {
        if ((status & STATUS_ENBO) == 0) {
            /* ac97 codec has comsumed a down stream slot */
            out->issues.counter--;
            hw_out++;
            out->mde[regular_idx(out->ihw,out->slots)].v = 0;   /* invalid mde entry */
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

{
    unsigned int a_00, a_04, a_08, a_0c;
    a_00 = status;
    a_04 = readl(0xb94b0004);
    a_08 = readl(0xb94b0008);
    a_0c = readl(0xb94b000c);
    printk("a_00 = %08x  a_04 = %08x  a_08 = %08x  a_0c = %08x\n", a_00, a_04, a_08, a_0c);
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


#ifdef CONFIG_APM
/******************************************************************************
 *  APM Callback function
 *  pm_dev_idle - when device closed or device paused
 *  pm_access   - once device invoke start_issue
 *  SUSPEND     - stop all device and wait dma done
 *                power off all device
 *  RESUME      - power on all device
 *                resume issue if device is opened
 ******************************************************************************/
static int pldsp_apm (struct pm_dev *pm_dev, pm_request_t rqst, void *data)
{
	if (pm_dev->state == (unsigned long)data)
		return 0;

	switch (rqst) {
    case PM_SUSPEND:
	    printk ("pldsp_apm: suspend request\n");

        pl_i2s_ins.out.suspend = 1;
        pl_i2s_ins.in.suspend = 1;
        stop_issue(&pl_i2s_ins.out);
        stop_issue(&pl_i2s_ins.in);

        wait_event(pl_i2s_ins.out.drain, get_dma_residue(DMA_PL_AC97_OUT) == 0);
        wait_event(pl_i2s_ins.in.drain, get_dma_residue(DMA_PL_AC97_IN) == 0);

        /* should power off something like that */


        break;

    case PM_RESUME:
	    printk ("pldsp_apm: resume request\n");

        /* should power on something here */

        if (pl_i2s_ins.out.state != ST_CLOSE)
            start_issue(&pl_i2s_ins.out);
        if (pl_i2s_ins.in.state != ST_CLOSE)
            start_issue(&pl_i2s_ins.in);

        pl_i2s_ins.out.suspend = 0;
        pl_i2s_ins.in.suspend = 0;

        wake_up(&pl_i2s_ins.out.apm_wait);
        wake_up(&pl_i2s_ins.in.apm_wait);
        break;

#if 0
    case PM_CHANGE_CLOCK:
	    pl1061sm_set_cts ();
		break;
#endif
	}
	return 0;
}
#endif /* CONFIG_APM */

/******************************************************************************
 *  Configure functions
 ******************************************************************************/
static void pl_dsp_set_channels(int channels, int output)
{
    u8 data;

    if (output) {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & ~I2SCONFIG_2_ADSM_MASK;
        if (channels == 2)
            writeb(data | I2SCONFIG_2_ADSM_STEREO, PL_AUDIO_CMD_AC97CONFIG_2);
        else
            writeb(data | I2SCONFIG_2_ADSM_MONO, PL_AUDIO_CMD_AC97CONFIG_2);
    } else {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & ~I2SCONFIG_3_AUSM_MASK;
        if (channels == 2)
            writeb(data | I2SCONFIG_3_AUSM_STEREO, PL_AUDIO_CMD_AC97CONFIG_3);
        else
            writeb(data | I2SCONFIG_3_AUSM_MONO_LEFT, PL_AUDIO_CMD_AC97CONFIG_3);
    }
}


static int pl_dsp_get_channels(int output)
{
    int channels;
    u8 data;

    if (output) {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & I2SCONFIG_2_ADSM_MASK;
        if (data == I2SCONFIG_2_ADSM_STEREO)
            channels = 2;
        else
            channels = 1;
    } else {
        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & I2SCONFIG_3_AUSM_MASK;
        if (data == I2SCONFIG_3_AUSM_STEREO)
            channels = 2;
        else
            channels = 1;
    }

    return channels;
}


/* PT5701 Sample rate coding table */

static int USBSampleCtrl[6][6] = {
                   /* 8MHz(0),  16MHz(1),   24MHz(2),   32MHz(3),   44.1MHz(4), 48MHz(5) */
 /* 8MHz    (0) */  { 0x06,     0x06,       0x06,       0x0b,       0x15,       0x04, },
 /* 16MHz   (1) */  { 0x06,     0x06,       0x06,       0x06,       0x06,       0x06, },
 /* 24MHz   (2) */  { 0x06,     0x06,       0x06,       0x06,       0x06,       0x06, },
 /* 32MHz   (3) */  { 0x07,     0x06,       0x06,       0x0c,       0x14,       0x05, },
 /* 44.1MHz (4) */  { 0x13,     0x06,       0x06,       0x0d,       0x11,       0x01, },
 /* 48MHz   (5) */  { 0x02,     0x06,       0x06,       0x1d,       0x10,       0x00, },
};

static inline int is_valid_rate(u16 rate)
{
    if (rate == 48000 || rate == 44100 || rate == 32000 || rate == 8000)
        return 1;

    return 1;
}

#define ADC_idx(srate)      ((srate/8000)-1)
#define DAC_idx(srate)      ((srate/8000)-1)

#define USB_PT_SAMPLE_RATE  0x1
#define MCLK                12000000


static u16 pl_dsp_set_samplerate(pl_i2s_t *pl_i2s, u16 rate, int output)
{
    int sr, leng;

    if (output) {
        if (!is_valid_rate(rate))
            rate = pl_i2s->osrate;
        else {
            /* set PT5701 PT_SAMPLE_RATE register */
            sr = USBSampleCtrl[ADC_idx(pl_i2s->israte)][DAC_idx(rate)] << 1;
// printk("set out sr = %x (%d, %d)\n", sr, pl_i2s->israte, rate);
            pt5701_write(PT_SAMPLE_RATE, USB_PT_SAMPLE_RATE | sr);
            /* set I2S Down stream channel length register */
            leng = (MCLK/rate) >> 1;
// printk("leng = %d\n", leng);
            writel(leng << I2SDOWN_DLENG_OFF, PL_AUDIO_CMD_I2S_DOWN);
            pl_i2s->osrate = rate;
        }
    } else {
        if (!is_valid_rate(rate))
            rate = pl_i2s->israte;
        else {
            /* set PT5701 PT_SAMPLE_RATE register */
            sr = USBSampleCtrl[ADC_idx(rate)][DAC_idx(pl_i2s->osrate)] << 1;
// printk("set in sr = %x (%d, %d)\n", sr, rate, pl_i2s->osrate);
            pt5701_write(PT_SAMPLE_RATE, USB_PT_SAMPLE_RATE | sr);
            /* set I2S Up stream channel length register */
            leng = (MCLK/rate) >> 1;
// printk("leng = %d\n", leng);
            writel(leng << I2SUP_ULENG_OFF, PL_AUDIO_CMD_I2S_UP);
            pl_i2s->israte = rate;
        }
    }
    return rate;
}

static u32 pl_dsp_get_samplerate(pl_i2s_t *pl_i2s, int output)
{
    u16 rate;
    if (output) {
        rate = pl_i2s->osrate;
    } else {
        rate = pl_i2s->israte;
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
        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & I2SCONFIG_2_ADPS_MASK;
        fmt = (data == I2SCONFIG_2_ADPS_16) ? 16 : 8;
    } else {
        unsign = readb(PL_AUDIO_CMD_FUNCTION_0) & FUNCTION_0_U_SIGN;
        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & I2SCONFIG_3_AUPS_MASK;
        fmt = (data == I2SCONFIG_3_AUPS_16) ? 16 : 8;
    }

    if (!unsign) {
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

        data = readb(PL_AUDIO_CMD_AC97CONFIG_2) & ~I2SCONFIG_2_ADPS_MASK;

        if (fmt == 8)
            writeb(data | I2SCONFIG_2_ADPS_8, PL_AUDIO_CMD_AC97CONFIG_2);
        else
            writeb(data | I2SCONFIG_2_ADPS_16, PL_AUDIO_CMD_AC97CONFIG_2);
    } else {
        data = readb(PL_AUDIO_CMD_FUNCTION_0) & ~FUNCTION_0_U_SIGN;
        if (sign)
            writeb(data, PL_AUDIO_CMD_FUNCTION_0);
        else
            writeb(data | FUNCTION_0_D_SIGN, PL_AUDIO_CMD_FUNCTION_0);

        data = readb(PL_AUDIO_CMD_AC97CONFIG_3) & ~I2SCONFIG_3_AUPS_MASK;

        if (fmt == 8)
            writeb(data | I2SCONFIG_3_AUPS_8, PL_AUDIO_CMD_AC97CONFIG_3);
        else
            writeb(data | I2SCONFIG_3_AUPS_16, PL_AUDIO_CMD_AC97CONFIG_3);
    }

    return format;
}

static void pl_calc_time_base(pl_i2s_t *pl_i2s, int output)
{
    int ch;
    int samples;
    int fmt;
    int bytes_per_sample;
    struct pl_i2s_info *info;

    ch = pl_dsp_get_channels(output);
    samples = pl_dsp_get_samplerate(pl_i2s, output);
    fmt = pl_dsp_get_format(output);
    if (fmt == AFMT_S16_LE || fmt == AFMT_U16_LE)
        bytes_per_sample = 2;
    else
        bytes_per_sample = 1;
    if (output)
        info = &pl_i2s->out.info;
    else
        info = &pl_i2s->in.info;

    info->bytes_per_sec = ch*bytes_per_sample*samples;
    info->bytes_per_hour = info->bytes_per_sec * 60 * 60;

}

/**
 * pl_dsp_getodelay: returns the number of unplayed bytes in the kernel buffer
 * NOTE: the hardware may play some samples which should be taken off
 */
static int pl_dsp_getodelay(struct pl_i2s_buf *out)
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
static int pl_dsp_getidelay(struct pl_i2s_buf *in)
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

static void pl_dsp_getoptr(struct pl_i2s_buf *out, count_info *info)
{
    info->bytes = out->info.tbytes;
    info->blocks = out->info.slots;
    out->info.slots = 0;
    info->ptr = out->info.bbytes;
}

static void pl_dsp_getiptr(struct pl_i2s_buf *in, count_info *info)
{
    info->bytes = in->info.tbytes;
    info->blocks = in->info.slots;
    in->info.slots = 0;
    info->ptr = in->info.bbytes;
}

#if 0
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
#endif


#ifdef CONFIG_PL_AC97_POWERDOWN
static void pl_dsp_power_up(pl_i2s_t *pl_i2s, int output)
{

    if (output) {
        pl_i2s->power |= PT_PW_VMIDSEL;
        pt5701_write(PT_POWER_1, pl_i2s->power);
        pt5701_write(PT_POWER_2, PT_PW_DACL);

    } else {
        pl_i2s->power &= PT_PW_VMIDSEL;
        pl_i2s->power |= PT_PW_ADCL;
        if (pl_i2s->recio) { /* mic */
            pl_i2s->power |= PT_PW_MICB | pl_i2s->micboost;
        }
        pt5701_write(PT_POWER_1, pl_i2s->power);
    }

}


static void pl_dsp_power_down(pl_i2s_t *pl_i2s, int output)
{
    if (output) {
        pl_i2s->power &= ~PT_PW_VMIDSEL;
        pt5701_write(PT_POWER_1, pl_i2s->power);
        pt5701_write(PT_POWER_2, 0);
    } else {
        pl_i2s->power &= PT_PW_VMIDSEL;
        pt5701_write(PT_POWER_1, pl_i2s->power);
    }

}

static void pl_dsp_rec_power_manager(pl_i2s_t *pl_i2s)
{

    pl_i2s->power &= PT_PW_VMIDSEL;
    pl_i2s->power |= PT_PW_ADCL;
    if (pl_i2s->recio) { /* mic */
        pl_i2s->power |= PT_PW_MICB | pl_i2s->micboost;
    }

    pt5701_write(PT_POWER_1,  pl_i2s->power);
}
#endif /* CONFIG_PL_AC97_POWERDOWN */

static int pl_dsp_setfragment(struct pl_i2s_buf *abuf, int valid, int nfrag, int frag_ord)
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

    if ((file->f_mode & FMODE_WRITE) && pl_i2s_ins.out.state != ST_CLOSE)
        return -EBUSY;

    if ((file->f_mode & FMODE_READ) && pl_i2s_ins.in.state != ST_CLOSE)
        return -EBUSY;

    /* downstream */
    if (file->f_mode & FMODE_WRITE) {
        pl_i2s_ins.out.state = ST_OPEN;
        pl_i2s_ins.out.start = 1;
#ifdef CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_up(&pl_i2s_ins, 1);
#else
        pt5701_write(PT_POWER_2, PT_PW_DACL);
#endif
        pl_dsp_set_channels(2, DIR_OUT);
        pl_dsp_set_format(fmt, DIR_OUT);
        pl_dsp_set_samplerate(&pl_i2s_ins,48000, DIR_OUT);
        memset(&pl_i2s_ins.out.info, 0, sizeof(struct pl_i2s_info));
        pl_calc_time_base(&pl_i2s_ins, DIR_OUT);
    }

    /* upstream */
    if (file->f_mode & FMODE_READ) {
        pl_i2s_ins.in.state = ST_OPEN;
        pl_i2s_ins.in.start = 1;
#ifdef CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_up(&pl_i2s_ins, 0);
#endif
        pl_dsp_set_channels(2, DIR_IN);
        pl_dsp_set_format(fmt, DIR_IN);
        pl_dsp_set_samplerate(&pl_i2s_ins,8000, DIR_IN);
        memset(&pl_i2s_ins.in.info, 0, sizeof(struct pl_i2s_info));
        pl_calc_time_base(&pl_i2s_ins, DIR_IN);
    }

    file->private_data = &pl_i2s_ins;

    return 0;
}

static int pl_dsp_release(struct inode *inode, struct file *file)
{
    int rc1 = 0, rc2 = 0;
    pl_i2s_t *pl_i2s = (pl_i2s_t *)file->private_data;

    if (file->f_mode & FMODE_WRITE) {
        rc1 = drain_out(&pl_i2s->out);
        pl_i2s->out.state = ST_CLOSE;
        clean_buffer(&pl_i2s->out);
#ifdef CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_down(pl_i2s, 1);
#else
        pt5701_write(PT_POWER_2, 0);
#endif
    }

    if (file->f_mode & FMODE_READ) {
        rc2 = drain_in(&pl_i2s->in);
        pl_i2s->in.state = ST_CLOSE;
        clean_buffer(&pl_i2s->in);
#ifdef CONFIG_PL_AC97_POWERDOWN
        pl_dsp_power_down(pl_i2s, 0);
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
    pl_i2s_t *pl_i2s = (pl_i2s_t *)file->private_data;
    struct pl_i2s_buf *out;
    int len, idx;
    int rc, bufsize;
    const char *p;

    if (pl_i2s == NULL)
        return -EINVAL;

    out = &pl_i2s->out;

    if (ppos != &file->f_pos)
        return -ESPIPE;

    if (!access_ok(VERIFY_READ, buffer, count))
        return -EFAULT;

#if AUDIO_MODULE_TRIM_DSP_WRITE_TO_DWORD
    count = count & ~3; /* trim to dword size */
#endif

    if ((count & 3) != 0)   /* should be double word size, limit by ac97 controller */
        return -EINVAL;

    if (out->suspend) {
        if (file->f_flags & O_NONBLOCK)
            return -EAGAIN;
        else
            wait_event(out->apm_wait, out->suspend != 0);
    }


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
    pl_i2s_t *pl_i2s = (pl_i2s_t *)file->private_data;
    struct pl_i2s_buf *in;
    int len, idx;
    int rc, bufsize;
    char *p;

    if (pl_i2s == NULL)
        return -EINVAL;

    in = &pl_i2s->in;

    if (ppos != &file->f_pos)
        return -ESPIPE;

    if (!access_ok(VERIFY_WRITE, buffer, count))
        return -EFAULT;


    if (in->suspend) {
        if (file->f_flags & O_NONBLOCK)
            return -EAGAIN;
        else
            wait_event(in->apm_wait, in->suspend != 0);
    }


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
    pl_i2s_t *pl_i2s = file->private_data;
    unsigned int mask = 0;

    if (file->f_mode & FMODE_WRITE)
        poll_wait(file, &pl_i2s->out.wait, wait);

    if (file->f_mode & FMODE_READ)
        poll_wait(file, &pl_i2s->in.wait, wait);


    /* is not FULL? slot available for writing */
    if ((file->f_mode & FMODE_WRITE) &&
        (ABS(pl_i2s->out.itail-pl_i2s->out.ihw) != pl_i2s->out.slots))
        mask |= (POLLOUT | POLLWRNORM);

    /* is not EMPTY? data is ready for copy */
    if ((file->f_mode & FMODE_READ) &&
        (!is_in_empty(&pl_i2s->in) ||
        /* BECAUSE, driver doesn't not issue upstream DMA at device openning. */
        /* It will be triggered at first read invokation. Here must consider  */
        /* this situation else application hang of starvation */
        (pl_i2s->in.ihead == pl_i2s->in.itail)))
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
    pl_i2s_t *pl_i2s = (pl_i2s_t *)file->private_data;
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
            validate_state(pl_i2s->out, ST_OPEN, -EINVAL);
            val = pl_dsp_set_format(val, DIR_OUT);
            pl_calc_time_base(pl_i2s, DIR_OUT);
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_i2s->in, ST_OPEN, -EINVAL);
            val = pl_dsp_set_format(val, DIR_IN);
            pl_calc_time_base(pl_i2s, DIR_IN);
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
            validate_state(pl_i2s->out, ST_OPEN, -EINVAL);
            pl_dsp_set_channels(channels, DIR_OUT);
            pl_calc_time_base(pl_i2s, DIR_OUT);
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_i2s->in, ST_OPEN, -EINVAL);
            pl_dsp_set_channels(channels, DIR_IN);
            pl_calc_time_base(pl_i2s, DIR_IN);
        }

        rc = put_user(val, (int *)arg);
        break;

    case SNDCTL_DSP_SPEED:
        get_user_ret(val, (int *)arg, -EFAULT);
        if (val < 0)
            return -EINVAL;
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_i2s->out, ST_OPEN, -EINVAL);
            val = pl_dsp_set_samplerate(pl_i2s, val, DIR_OUT);
            pl_calc_time_base(pl_i2s, DIR_OUT);
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_i2s->in, ST_OPEN, -EINVAL);
            val = pl_dsp_set_samplerate(pl_i2s, val, DIR_IN);
            pl_calc_time_base(pl_i2s, DIR_IN);
        }

        rc = put_user(val, (int *)arg);
        break;

    case SNDCTL_DSP_SYNC:
        if (file->f_mode & FMODE_WRITE) {
            drain_out(&pl_i2s->out);
            pl_i2s->out.state = ST_OPEN;
        }
        break;

    case SNDCTL_DSP_SETDUPLEX:
        return 0;
    case SNDCTL_DSP_RESET:
        if (file->f_mode & FMODE_READ && pl_i2s->in.state != ST_OPEN) {   /* 003 */
            validate_state(pl_i2s->in, ST_RW, -EINVAL);
            stop_issue(&pl_i2s->in);
#if AUDIO_MODULE_63
            writeb(0, PL_AUDIO_CMD_COMMAND_2);  /* force hardware to reset downstream */
#else
            writeb(COMMAND_2_DMAU, PL_AUDIO_CMD_COMMAND_2);  /* force hardware to reset downstream */
#endif
            drain_in(&pl_i2s->in);
            clean_buffer(&pl_i2s->in);
            pl_i2s->in.state = ST_OPEN;
        }
        if (file->f_mode & FMODE_WRITE && pl_i2s->out.state != ST_OPEN) { /* 003 */
            validate_state(pl_i2s->out, ST_RW, -EINVAL);
            stop_issue(&pl_i2s->out);
#if AUDIO_MODULE_63
            writeb(0, PL_AUDIO_CMD_COMMAND_3);  /* force hardware to reset downstream */
#else
            writeb(COMMAND_3_DMAD, PL_AUDIO_CMD_COMMAND_3);  /* force hardware to reset downstream */
#endif
            drain_out(&pl_i2s->out);
            clean_buffer(&pl_i2s->out);
            pl_i2s->out.state = ST_OPEN;
        }
        break;
    case SNDCTL_DSP_POST:
        if (file->f_mode & FMODE_WRITE)
            pause_issue(&pl_i2s->out);
        break;

    case SNDCTL_DSP_GETOSPACE:
        if (!(file->f_mode & FMODE_WRITE))
            return -EINVAL;

        if (pl_i2s->out.ihw == pl_i2s->out.itail)
            info.fragments = pl_i2s->out.slots;
        else {
            info.fragments = regular_idx(pl_i2s->out.ihw, pl_i2s->out.slots) -
                             regular_idx( pl_i2s->out.itail, pl_i2s->out.slots);
            if (info.fragments < 0)
                info.fragments = pl_i2s->out.slots + info.fragments;
        }

        info.fragstotal = pl_i2s->out.slots;
        info.fragsize = 1 << pl_i2s->out.order;
        info.bytes = info.fragments * info.fragsize;
        rc = copy_to_user((void *)arg, &info, sizeof(audio_buf_info));
        break;

    case SNDCTL_DSP_GETISPACE:
        if (!(file->f_mode & FMODE_READ))
            return -EINVAL;

        if (pl_i2s->in.ihw == pl_i2s->in.ihead)
            info.fragments = 0;
        else {
            info.fragments = regular_idx(pl_i2s->in.ihw, pl_i2s->in.slots) -
                             regular_idx( pl_i2s->in.ihead, pl_i2s->in.slots);
            if (info.fragments <= 0)
                info.fragments = pl_i2s->in.slots + info.fragments;
        }

        info.fragstotal = pl_i2s->in.slots;
        info.fragsize = 1 << pl_i2s->in.order;
        info.bytes = info.fragments * info.fragsize;
        rc = copy_to_user((void *)arg, &info, sizeof(audio_buf_info));
        break;

    case SNDCTL_DSP_SETFRAGMENT:
        get_user_ret(val, (int *)arg, -EFAULT);
        if (file->f_mode & FMODE_WRITE) {
            validate_state(pl_i2s->out, ST_OPEN, -EINVAL);
            rc = pl_dsp_setfragment(&pl_i2s->out, 0, ((unsigned int)val) >> 16, val & 0xffff );
        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_i2s->in, ST_OPEN, -EINVAL);
            rc = pl_dsp_setfragment(&pl_i2s->in, 1, ((unsigned int)val) >> 16, val & 0xffff );
        }
        break;
    case SNDCTL_DSP_GETOPTR:
        if (!(file->f_mode & FMODE_WRITE))
            return -EINVAL;
        pl_dsp_getoptr(&pl_i2s->out, &cinfo);
        rc = copy_to_user((void *)arg, &cinfo, sizeof(count_info));
        break;
    case SNDCTL_DSP_GETIPTR:
        if (!(file->f_mode & FMODE_READ))
            return -EINVAL;
        pl_dsp_getiptr(&pl_i2s->in, &cinfo);
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
            validate_state(pl_i2s->out, ST_OPEN, -EINVAL);

            if (val & PCM_ENABLE_OUTPUT)
                start_issue(&pl_i2s->out);
            else
                stop_issue(&pl_i2s->out);

        }
        if (file->f_mode & FMODE_READ) {
            validate_state(pl_i2s->in, ST_OPEN, -EINVAL);

            if (val & PCM_ENABLE_INPUT)
                start_issue(&pl_i2s->in);
            else
                stop_issue(&pl_i2s->in);
        }
        break;
    case SNDCTL_DSP_GETTRIGGER:
        if (file->f_mode & FMODE_WRITE) {
            if (pl_i2s->out.start)
                val |= PCM_ENABLE_OUTPUT;
        }
        if (file->f_mode & FMODE_READ) {
            if (pl_i2s->in.start)
                val |= PCM_ENABLE_INPUT;
        }
        break;

    case SNDCTL_DSP_GETBLKSIZE:
        if (file->f_mode & FMODE_WRITE)
            val = 1 << pl_i2s->out.order;
        else if (file->f_mode & FMODE_READ)
            val = 1 << pl_i2s->in.order;
        rc = put_user(val, (int*)arg);
        break;

    case SOUND_PCM_READ_RATE:
        if (file->f_mode & FMODE_WRITE)
            val = pl_dsp_get_samplerate(pl_i2s, DIR_OUT);
        else if (file->f_mode & FMODE_READ)
            val = pl_dsp_get_samplerate(pl_i2s, DIR_IN);
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
            val = pl_dsp_getodelay(&pl_i2s->out);
        else
            val = 0;

        rc = put_user(val, (int *)arg);
        break;
    /* Prolific speical audio ioctl options */
    case SNDCTL_DSP_GETIDELAY:
        if (file->f_mode & FMODE_READ)
            val = pl_dsp_getidelay(&pl_i2s->in);
        else
            val = 0;

        rc = put_user(val, (int *)arg);
        break;


    case PL_SNDCTL_DSP_SYMALR_REQ :
        if (file->f_mode & FMODE_WRITE) {
            PLSymbolAlarm SymbolAlarm;
            int ord;

            validate_state(pl_i2s->out, ST_OPEN, -EINVAL);
            rc = copy_from_user(&SymbolAlarm, (void *)arg, sizeof(PLSymbolAlarm));
            if (rc < 0)
                return -EFAULT;
            if (SymbolAlarm.nPID == 0)
                SymbolAlarm.nPID = current->pid;
            if (SymbolAlarm.nMaxBufferSize > 0 && SymbolAlarm.nMaxBufferAmount > 0) {
                ord = get_order_by_size(SymbolAlarm.nMaxBufferSize);
                rc = pl_dsp_setfragment(&pl_i2s->out, 0, SymbolAlarm.nMaxBufferAmount, ord);
                if (rc < 0)
                     break;
            }
            SymbolAlarm.nMaxBufferAmount = pl_i2s->out.slots;
            SymbolAlarm.nMaxBufferSize = 1 << pl_i2s->out.order;
            pl_i2s->out.threshold = (SymbolAlarm.nFrequency > SymbolAlarm.nMaxBufferSize) ?
                                    SymbolAlarm.nMaxBufferSize : SymbolAlarm.nFrequency;
            pl_i2s->out.alarmpid = SymbolAlarm.nPID;
            rc = copy_to_user((void *)arg, &SymbolAlarm, sizeof(PLSymbolAlarm));
        } else
            return -EINVAL;
        break;

    case PL_SNDCTL_DSP_SYMALR_FREQUENCY:
        if (file->f_mode & FMODE_WRITE) {
            get_user_ret(val, (int *)arg, -EFAULT);
            if (val > (1 << pl_i2s->out.order) || val < 0)
                return -EINVAL;
            pl_i2s->out.threshold = val;
            if (pl_i2s->out.alarmpid == 0)
                pl_i2s->out.alarmpid = current->pid;
        } else
            return -EINVAL;
         break;
    case PL_SNDCTL_DSP_SYMALR_ACTIVE :
        if (file->f_mode & FMODE_WRITE) {
            get_user_ret(val, (int *)arg, -EFAULT);
            switch(val) {
            case PL_SYMBOL_ALARM_QUERY:
                val = (pl_i2s->out.alarm) ? PL_SYMBOL_ALARM_ENABLE : PL_SYMBOL_ALARM_DISABLE;
                rc = put_user(val, (int *)arg);
                break;

            case PL_SYMBOL_ALARM_DISABLE:
                pl_i2s->out.alarm = 0;
                break;
            case PL_SYMBOL_ALARM_ENABLE:
                pl_i2s->out.alarm = 1;
                break;
            default:
                rc = -EINVAL;
            }
        } else
            return -EINVAL;
        break;

    case PL_SNDCTL_DSP_SYMALR_REL :
        if (file->f_mode & FMODE_WRITE) {
            pl_i2s->out.threshold = (1 << pl_i2s->out.order);
            pl_i2s->out.alarm = 0;
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
    if (pl_i2s_ins.dev_mixer != minor)
        return -ENODEV;

    file->private_data = &pl_i2s_ins;
    return 0;
}

#define PT5701_STEREO_MASK (SOUND_MASK_VOLUME|SOUND_MASK_PCM|SOUND_MASK_IGAIN)
#define PT5701_RECORD_MASK (SOUND_MASK_MIC|SOUND_MASK_LINE)
#define PT5701_SUPPORTED_MASK (PT5701_STEREO_MASK)

#define  AC97_MIC_VOL             0x000e      // MIC Input (mono)


static int pt5701_set_mixer(pl_i2s_t *pl_i2s, unsigned int channel, unsigned int val)
{
    int right, left, mute;

	/* cleanse input a little */
	right = ((val >> 8)  & 0xff);
	left = (val  & 0xff) ;
    mute = val & 0x80000;

	if (right > 100) right = 100;
	if (left > 100) left = 100;

    switch (channel) {
    case SOUND_MIXER_VOLUME:
        pl_i2s->mixer_state[0] = mute | (right << 8) | left;

        left = ((100-left)*127 + 50)/100;
        right = ((100-right)*127 + 50)/100;

        pt5701_write(PT_LOUT_VOL, left);
        pt5701_write(PT_ROUT_VOL, right);

        if (mute)
            pt5701_write(PT_DAC_MUTE, 0x8);
        else
            pt5701_write(PT_DAC_MUTE, 0x0);
        break;
    case SOUND_MIXER_PCM:
        pl_i2s->mixer_state[1] = mute | (right << 8) | left;

        left = (left*255 + 50)/100;
        right =(right*255 + 50)/100;

        pt5701_write(PT_DAC_L_VOL, left);
        pt5701_write(PT_DAC_R_VOL, right);

        pt5701_dac_left = left;
        pt5701_dac_right = right;

        break;

    case SOUND_MIXER_IGAIN:
        pl_i2s->mixer_state[2] = mute | (right << 8) | left;
#if 1
        left = (left*63+50)/100;
        pt5701_write(PT_PGA, left);
#else
        left = (((100 - left)*15)+50)/100;
        right = (((100 - right)*15)+50)/100;
        pt5701_write(PT_ADC_L_VOL, left);
        pt5701_write(PT_ADC_R_VOL, right);
#endif
        break;
    }
    return 0;
}

static int pt5701_get_mixer(pl_i2s_t *pl_i2s, unsigned int channel)
{
    unsigned int val = 0;

    switch (channel) {
    case SOUND_MIXER_VOLUME:
        val = pl_i2s->mixer_state[0];
        break;

    case SOUND_MIXER_PCM:
        val = pl_i2s->mixer_state[1];
        break;
    case SOUND_MIXER_IGAIN:
        val = pl_i2s->mixer_state[2];
        break;
    }
    return val;
}






static int pl_mixer_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
    pl_i2s_t *pl_i2s = (pl_i2s_t *)file->private_data;
    unsigned int val;
    unsigned int i;


	if (cmd == SOUND_MIXER_INFO)
	{
		mixer_info info;
		strncpy(info.id, "I2S", sizeof(info.id));
		strncpy(info.name, "PT5701", sizeof(info.name));
		info.modify_counter = pl_i2s->modcnt;
		if (copy_to_user((void *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}
	if (cmd == SOUND_OLD_MIXER_INFO)
	{
		_old_mixer_info info;
		strncpy(info.id, "I2S", sizeof(info.id));
		strncpy(info.name, "PT5701", sizeof(info.name));
		if (copy_to_user((void *)arg, &info, sizeof(info)))
			return -EFAULT;
		return 0;
	}


    if (_IOC_TYPE(cmd) == 'A' && _SIOC_SIZE(cmd) == sizeof(int))
    {
        int reg;
        reg = _IOC_NR(cmd);

    	if (_SIOC_DIR(cmd) == _SIOC_READ) {
            if (reg == AC97_MIC_VOL) {
                if (pl_i2s->micboost)
                    val = 1 << 6;
                else
                    val = 0;
               return put_user(val, (int *)arg);
            }
        } else {
            if (get_user(val, (int *)arg))
			    return -EFAULT;

            if (reg == AC97_MIC_VOL) {
                if (val & (1 << 6))
                    pl_i2s->micboost = (1 << 4);
                else
                    pl_i2s->micboost = 0;

#ifdef CONFIG_PL_AC97_POWERDOWN
                pl_dsp_rec_power_manager(pl_i2s);
#endif
                return 0;
            }
        }

        return -EINVAL;
    }

	if (_IOC_TYPE(cmd) != 'M' || _SIOC_SIZE(cmd) != sizeof(int))
		return -EINVAL;

	if (cmd == OSS_GETVERSION)
		return put_user(SOUND_VERSION, (int *)arg);

	if (_SIOC_DIR(cmd) == _SIOC_READ)
	{
		switch (_IOC_NR(cmd)) {
		case SOUND_MIXER_RECSRC: /* give them the current record source */
			if (pt5701_read(PT_ADC_R, &val))
                val = 0;
            else
                val  = (val & 0xc0) ? SOUND_MASK_MIC : SOUND_MASK_LINE;
			break;

		case SOUND_MIXER_DEVMASK: /* give them the supported mixers */
			val = PT5701_SUPPORTED_MASK;
			break;

		case SOUND_MIXER_RECMASK: /* Arg contains a bit for each supported recording source */
			val = PT5701_RECORD_MASK;
			break;

		case SOUND_MIXER_STEREODEVS: /* Mixer channels supporting stereo */
			val = PT5701_STEREO_MASK;
			break;

		case SOUND_MIXER_CAPS:
			val = SOUND_CAP_EXCL_INPUT;
			break;

		default: /* read a specific mixer */
			i = _IOC_NR(cmd);

            if (!( (1 << i) & PT5701_SUPPORTED_MASK))
                val = 0;
            else
                val = pt5701_get_mixer(pl_i2s, i);

 			break;
		}
		return put_user(val, (int *)arg);
	}

	if (_SIOC_DIR(cmd) == (_SIOC_WRITE|_SIOC_READ))
	{
		pl_i2s->modcnt++;
		if (get_user(val, (int *)arg))
			return -EFAULT;

		switch (_IOC_NR(cmd))
		{
		case SOUND_MIXER_RECSRC: /* Arg contains a bit for each recording source */
			if (!val) return 0;
			if (!(val &= PT5701_RECORD_MASK)) return -EINVAL;
            if (val == SOUND_MASK_MIC) {
                pt5701_write(PT_ADC_L, 0x40 | pl_i2s->micboost);
                pt5701_write(PT_ADC_R, 0x40);
#ifdef CONFIG_PL_AC97_POWERDOWN
                pl_dsp_rec_power_manager(pl_i2s);
#endif

            } else if (val == SOUND_MASK_LINE) {
                pt5701_write(PT_ADC_L, pl_i2s->micboost);
                pt5701_write(PT_ADC_R, 0);
#ifdef CONFIG_PL_AC97_POWERDOWN
                pl_dsp_rec_power_manager(pl_i2s);
#endif
            } else
                return -EINVAL;

			return 0;


		default: /* write a specific mixer */
			i = _IOC_NR(cmd);

            if (!( (1 << i) & PT5701_SUPPORTED_MASK))
                return 0;

			pt5701_set_mixer(pl_i2s, i, val);

			return 0;
		}
	}

	return -EINVAL;

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
static int pl_i2s_reg_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    int i;
    int value;

    for (i = 0; i < 0x22; i++) {
        if (pt5701_read(i, &value))
            value = 0xfff;
        len += sprintf(page+len, "0x%02x = 0x%03x   ", i, value);
        if (i % 4 == 3)
            len += sprintf(page+len, "\n");
    }

    len += sprintf(page+len, "\n");

    return len;
}

static int pl_i2s_read_proc(char *page, char **start, off_t off, int count, int *eof, void *data)
{
    int len = 0;
    pl_i2s_t *pl_i2s = data;
    struct pl_i2s_info *out_info = &pl_i2s->out.info;
    struct pl_i2s_info *in_info = &pl_i2s->in.info;
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


static void __init pl_i2s_init_proc(void)
{
    struct proc_dir_entry *audio_proc;

    audio_proc = proc_mkdir("audio", 0);
    if (audio_proc == NULL)
        goto EXIT;

    audio_proc->owner = THIS_MODULE;
    create_proc_read_entry("i2s", 0, audio_proc, pl_i2s_reg_read_proc, &pl_i2s_ins);
    create_proc_read_entry("info", 0, audio_proc, pl_i2s_read_proc, &pl_i2s_ins);

EXIT:
    return;
}

struct i2c_adapter *i2c_adap = NULL;
#define PT5701_ADDR     0x1a

static int pt5701_write(int reg, int value)
{
	struct i2c_msg msg;
    unsigned long buf;
    int ret;

    msg.addr  = PT5701_ADDR;
    msg.flags = 0;
    msg.len = 2;
    buf = ((reg << 1) + ((value >> 8) & 0x1)) + ((value & 0xff) << 8);

    (const char *)msg.buf = &buf;
    down(&i2c_adap->bus);
    ret = i2c_adap->algo->master_xfer(i2c_adap,&msg,1);
	up(&i2c_adap->bus);

    return  ret;
}

static int pt5701_read(int reg, int *value)
{
	struct i2c_msg msg;
    unsigned long buf;
	int ret;
    int i;
    int rc = -1;

    *value = 0;

	msg.addr  = PT5701_ADDR;
	msg.flags = I2C_M_RD;
	msg.len = 2;
	msg.buf = (char *)&buf;

	down(&i2c_adap->bus);

    for (i = 0;i < 0x22; i++) {
    	ret = i2c_adap->algo->master_xfer(i2c_adap,&msg,1);
        if (ret != 2)
            break;

        if (reg == ((buf & 0xff) >> 1)) {
            *value = ((buf & 1) << 8) + ((buf & 0xff00) >> 8);
            rc = 0;
            break;
        }
    }
	up(&i2c_adap->bus);

    return rc;
}


extern struct i2c_adapter * i2c_get_adapter_by_id(int id);
extern void __init i2c_init_all(void);
extern int __init i2c_dev_init(void);
extern int __init i2c_61_init(void);





static int __init pl_i2s_init(void)
{
    int rc = 0;
    int value;

#if AUDIO_MODULE_63
    printk("Prolific PT5701 driver version 2.1 for 63 and 29 Audio Module 2006/03/21\n");
#else
    printk("Prolific PT5701 driver version 1.0 2004/11/17\n");
#endif

    memset(&pl_i2s_ins, 0, sizeof(pl_i2s_t));

    /* Init I2S Interface */

    /* ac97 & I2S controller reset */
    writeb(0, PL_AUDIO_CMD_COMMAND_1);
    writeb(COMMAND_1_B_RSTN, PL_AUDIO_CMD_COMMAND_1);

    /* reset ac97 function register */
    writel(0, PL_AUDIO_CMD_FUNCTION_0);
    writeb(FUNCTION_1_VRA, PL_AUDIO_CMD_FUNCTION_1);

    /* set internal clock and divider enable */
#if AUDIO_MODULE_63
    writel(0x04, PL_AUDIO_CMD_AC97CLOCK);
    writel(0x02020202, 0xBB000410); /* ckdiv_ac97 ?? */
#else
    writel(0x02, PL_AUDIO_CMD_AC97CLOCK);
    writel(0x02020202, 0xBB000410); /* ckdiv_ac97 ?? */
#endif

    /* program GPIO pins to all output*/
    writeb(0, PL_AUDIO_CMD_GPIO_C_ENB);     /* select pin as I2S function */
    writeb(0x0e, PL_AUDIO_CMD_GPIO_C_OE);   /* for I2S should be 01110 external clock */
    // writeb(0x0f, PL_AUDIO_CMD_GPIO_C_OE);   /* for I2S should be 01111 internal clock */

    writel(0x03030303, 0xBB000410); /* refer USB phy 48MHz and divid 4 to 12MHz */

    /* Init I2C interface */

    i2c_init_all();
    i2c_dev_init();
    i2c_61_init();


    /* Init PT5701 by I2C command */
    i2c_adap = i2c_get_adapter_by_id(0);

    pt5701_write(0x0f, 0x100);  /* reset */
    de_sound_burst_start();


#ifdef CONFIG_PL_AC97_POWERDOWN
    pt5701_write(PT_POWER_1, 0);
    pt5701_write(PT_POWER_2, 0);
#else
    pt5701_write(PT_POWER_1, PT_PW_VMIDSEL | PT_PW_AINL | PT_PW_ADCL | PT_PW_MICB);
    pt5701_write(PT_POWER_2, 0);
#endif
    pt5701_write(PT_DAC_MUTE, PT_MUTE); /* MUTE PT5701 */
    // pt5701_write(0x05, 0x00);   /* ADC and DAC control (mute) */
    pt5701_write(0x07, 0x02);   /* Audio interface */
    pt5701_write(0x08, 0x01);   /* Sample Rate, 0x01->48K, 0x23->44.1K */
    pt5701_write(0x18, 0x00);   /* Additional Conrtol */
#if !defined(CONFIG_DE_SOUND_BURST)
    pt5701_write(0x0a, pt5701_dac_left);   /* left channel digital volume */
    pt5701_write(0x0b, pt5701_dac_right);   /* right channel digital volume */
#endif



#if 0
{
int i;
    for (i = 0; i < 0x22; i++) {
        pt5701_read(i, &value);
        printk("0x%02x = 0x%03x\n", i, value);
    }
}
#endif

    pt5701_read(0x00, &value);
    if (value == 0) {
        printk("PT5701 cannot be detected\n");
        return 0;
    }


    /* external clock */
    writel(0x01, PL_AUDIO_CMD_AC97CLOCK);

    /* set i2s mode */
    writeb(COMMAND_0_I2S, PL_AUDIO_CMD_COMMAND_0);
    writel(0, PL_AUDIO_CMD_FUNCTION_0);
    writeb(I2S_WSO_MODE | I2S_WSI_MODE, PL_AUDIO_CMD_FUNCTION_2);   /* enable I2S up/down format */


    /*
     * Register a dsp device
     */

    pl_i2s_ins.dev_mixer = register_sound_mixer(&pl_mixer_ops, -1);
    if (pl_i2s_ins.dev_mixer < 0) {
        printk("unable to register I2S mixer, aborting\n");
        rc = pl_i2s_ins.dev_mixer;
        goto EXIT;
    }


    pl_i2s_ins.dspo = register_sound_dsp(&pl_dsp_ops, -1);
    if (pl_i2s_ins.dspo < 0) {
        printk("unable to register dsp output device, aborting\n");
        rc = pl_i2s_ins.dspo;
        goto EXIT;
    }

    pl_i2s_ins.dspi = register_sound_dsp(&pl_dsp_ops, -1);
    if (pl_i2s_ins.dspi < 0) {
        printk("unable to register dsp output device, aborting\n");
        rc = pl_i2s_ins.dspi;
        goto EXIT;
    }


    /* initial out/in buffer */
    pl_dsp_setfragment(&pl_i2s_ins.out, 0, DEFAULT_OUT_BUF_SLOTS, DEFAULT_OUT_BUF_SLOT_SIZE_ORDER);
    init_waitqueue_head(&pl_i2s_ins.out.wait);
    init_waitqueue_head(&pl_i2s_ins.out.drain);
    init_waitqueue_head(&pl_i2s_ins.out.apm_wait);
    atomic_set(&pl_i2s_ins.out.issues, 0);
    pl_i2s_ins.out.issue = issue_out;

    pl_dsp_setfragment(&pl_i2s_ins.in, 1, DEFAULT_IN_BUF_SLOTS, DEFAULT_IN_BUF_SLOT_SIZE_ORDER);
    init_waitqueue_head(&pl_i2s_ins.in.wait);
    init_waitqueue_head(&pl_i2s_ins.in.drain);
    init_waitqueue_head(&pl_i2s_ins.in.apm_wait);
    atomic_set(&pl_i2s_ins.in.issues, 0);
    pl_i2s_ins.in.issue = issue_in;

    /* initial default sample rate */
    pl_i2s_ins.osrate = 48000;
    pl_i2s_ins.israte = 8000;
    pl_i2s_ins.micboost = 0;
    pl_i2s_ins.modcnt = 0;
    pl_i2s_ins.power = 0;

    pl_i2s_ins.mixer_state[0] = (12 << 8) | 12;
    pl_i2s_ins.mixer_state[1] = (50 << 8) | 50;
    pl_i2s_ins.mixer_state[2] = (36<<8) | 36;


    /* init dma channel */
    rc = request_dma(DMA_PL_AC97_OUT, "I2S DS");
    if (rc < 0)
        goto EXIT;

    set_dma_mde_base(DMA_PL_AC97_OUT, (const char *)pl_i2s_ins.out.mde);
    set_dma_mode(DMA_PL_AC97_OUT, DMA_MODE_WRITE);
#if AUDIO_MODULE_63_DMA_LINE_BUG
    set_dma_burst_length(DMA_PL_AC97_OUT, 0);
#else
    set_dma_burst_length(DMA_PL_AC97_OUT, DMA_BURST_LINE);
#endif

    rc = request_dma(DMA_PL_AC97_IN, "I2S US");
    if (rc < 0)
        goto EXIT;

    set_dma_mde_base(DMA_PL_AC97_IN, (const char *)pl_i2s_ins.in.mde);
    set_dma_mode(DMA_PL_AC97_IN, DMA_MODE_READ);
#if AUDIO_MODULE_63_DMA_LINE_BUG
    set_dma_burst_length(DMA_PL_AC97_IN, 0);
#else
    set_dma_burst_length(DMA_PL_AC97_IN, DMA_BURST_LINE);
#endif

    /* init irq handler */
    rc = request_irq(IRQ_PL_AC970, pl_i2s_handler, 0, "Audio I2S", &pl_i2s_ins);
    if (rc < 0)
        goto EXIT;

    /* ac97 controller enable interrupt */
#if AUDIO_MODULE_63
    writeb(COMMAND_1_B_RSTN | COMMAND_1_INTE_DNDA | COMMAND_1_INTE_UPDA, PL_AUDIO_CMD_COMMAND_1);
#else
    writeb(COMMAND_0_I2S | COMMAND_0_INTE, PL_AUDIO_CMD_COMMAND_0);
#endif

    /* create proc entry */
    pl_i2s_init_proc();

#ifdef CONFIG_APM
	if (!(pldsp_pm_dev = pm_register (PM_PL_DEV, PM_PL_AC97, pldsp_apm)))
		printk ("Audio driver failed to register an APM device\n");

    pm_dev_idle(pldsp_pm_dev);
#endif /* CONFIG_APM */


    return 0;
EXIT:
    if (pl_i2s_ins.dspi > 0)
        unregister_sound_dsp(pl_i2s_ins.dspi);

    if (pl_i2s_ins.dspo > 0)
        unregister_sound_dsp(pl_i2s_ins.dspo);

    if (pl_i2s_ins.dev_mixer > 0)
        unregister_sound_dsp(pl_i2s_ins.dev_mixer);

    return rc;
}

module_init(pl_i2s_init);

#ifdef CONFIG_MODULES
static void __exit pl_i2s_clean(void)
{
    printk(KERN_INFO "pl_i2s: unloading\n");

}
module_exit(pl_i2s_clean);
#endif


/**
 * Grammar:
 *    pl_i2s=[outbuf(size_order,number)][,inbuf(size_order,number)]
 *    the size is a fragment order size, it can be 256, 512, 1024, 2048, 4096(defult), 8192 and 16384
 *    the number is a maxiumn fragments, it can be 4 ~ 64
 */
static int __init pl_i2s_setup(char *options)
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

__setup("pl_i2s=", pl_i2s_setup);

MODULE_AUTHOR("Jedy Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("PL1061 i2s audio driver");

