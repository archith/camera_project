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

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-proc.h>
#include <linux/init.h>
#include <asm/io.h>
#define LM_DATE "20031120"
#define LM_VERSION "1.0.1"

MODULE_LICENSE("GPL");

/* Addresses to scan */
static unsigned short normal_i2c[] = { SENSORS_I2C_END };
static unsigned short normal_i2c_range[] = {0x20, 0x22, SENSORS_I2C_END };
static unsigned int normal_isa[] = { SENSORS_ISA_END };
static unsigned int normal_isa_range[] = { SENSORS_ISA_END };

/* Insmod parameters */
SENSORS_INSMOD_1(vt1622);

/* VT1622 A/AM Register */
#define VT_OUTPUT_SELECT    0x02
#define VT_LUMA_FILTER      0x03
#define VT_OUTPUT_MODE      0x04
#define VT_HPOS             0x08
#define VT_VPOS             0x09
#define VT_POWER_MANG       0x0e
#define VT_OVERFLOW         0x1c
#define VT_VID              0x1b

/* Each client has this additional data */
struct vt1622_data {
    int sysctl_id;

    struct semaphore update_lock;
    char valid;         /* !=0 if following fields are valid */

    u16 res[2];         /* Resolution XxY */
    u8 ntsc;            /* 1=NTSC, 0=PAL */
    u8 svideo;          /* output format: 2=YCbCr, 1=SVIDEO, 0=Composite */
    u8 colorbar;        /* colorbar 0: disabled 1: enabled */
    u8 laced;           /* 0 interlaced, 1 non-interlaced, 2 progressive */
    u16 hvpos[2][2];       /* h v position */
};

static int vt1622_attach_adapter(struct i2c_adapter *adapter);
static int vt1622_detect(struct i2c_adapter *adapter, int address,
            unsigned short flags, int kind);
static void vt1622_init_client(struct i2c_client *client);
static int vt1622_detach_client(struct i2c_client *client);
static inline int vt1622_read_next(struct i2c_client *client);
static int vt1622_read_value(struct i2c_client *client, u8 reg);
static int vt1622_write_value(struct i2c_client *client, u8 reg, u8 value);
static void vt1622_write_values(struct i2c_client *client, const int nreg, u8 *values);
static void vt1622_ntsc(struct i2c_client *client, int operation,
               int ctl_name, int *nrels_mag, long *results);
static void vt1622_res(struct i2c_client *client, int operation,
              int ctl_name, int *nrels_mag, long *results);
static void vt1622_svideo(struct i2c_client *client, int operation,
                int ctl_name, int *nrels_mag, long *results);
static void vt1622_colorbar(struct i2c_client *client, int operation,
                int ctl_name, int *nrels_mag, long *results);
static void vt1622_hvpos(struct i2c_client *client, int operation,
                int ctl_name, int *nrels_mag, long *results);
static void vt1622_laced(struct i2c_client *client, int operation,
                int ctl_name, int *nrels_mag, long *results);
static void vt1622_update_client(struct i2c_client *client);
static void vt1622_update_res(struct i2c_client *client);


/* This is the driver that will be inserted */
static struct i2c_driver vt1622_driver = {
    .owner      = THIS_MODULE,
    .name       = "vt1622 video-output chip driver",
    .id         = I2C_DRIVERID_VT1622,
    .flags      = I2C_DF_NOTIFY,
    .attach_adapter = vt1622_attach_adapter,
    .detach_client  = vt1622_detach_client,
};

/* -- SENSORS SYSCTL START -- */
#define VT1622_SYSCTL_NTSC      1000
#define VT1622_SYSCTL_RES       1001
#define VT1622_SYSCTL_SVIDEO    1002
#define VT1622_SYSCTL_COLORBAR  1003
#define VT1622_SYSCTL_HVPOS     1004
#define VT1622_SYSCTL_LACED     1005

/* -- SENSORS SYSCTL END -- */

/* These files are created for each detected vt1622. This is just a template;
   though at first sight, you might think we could use a statically
   allocated list, we need some way to get back to the parent - which
   is done through one of the 'extra' fields which are initialized
   when a new copy is allocated. */
static ctl_table vt1622_dir_table_template[] = {
    {VT1622_SYSCTL_NTSC, "ntsc", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &vt1622_ntsc},
    {VT1622_SYSCTL_RES, "res", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &vt1622_res},
    {VT1622_SYSCTL_SVIDEO, "svideo", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &vt1622_svideo},
    {VT1622_SYSCTL_COLORBAR, "colorbar", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &vt1622_colorbar},
    {VT1622_SYSCTL_HVPOS, "h.v.pos", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &vt1622_hvpos},
    {VT1622_SYSCTL_LACED, "laced", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &vt1622_laced},
    {0}
};


static u8 registers_800_480[] = {
 0x00, 0x20, 0x00, 0x01, 0x03, 0x88, 0x20, 0x79, 0xb0, 0x00, 0x7a, 0x14, 0x4e, 0x54, 0x07, 0x8f,
 0x00, 0x00, 0x14, 0x08, 0x28, 0x50, 0x7d, 0xe8, 0xa1, 0x1c, 0xee, 0x10, 0x00, 0x80, 0x00, 0x33,
 0x1d, 0x08, 0x99, 0x73, 0x10, 0x56, 0x31, 0x90, 0x51, 0x00, 0x00, 0xa3, 0x29, 0x5d, 0xc3, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x51,
 0xf8, 0x1f, 0x33, 0x0c, 0x02, 0xf8, 0x1f, 0x50, 0x33, 0x88, 0xcb, 0xd8, 0xf0, 0x07, 0x00, 0x5a,
 0x04, 0x00, 0x00, 0xf8, 0x03, 0x73, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


/*******************
720x576, 27.5MHz, PAL, no overscan compensation.
This mode should be use for digital video, DVD playback etc.

NOTE: This mode for PAL, see 720x480 for an equivalent NTSC mode

Compatible fbset modeline:

    Mode        "720x576-vt1622"
        geometry    720 576 720 576 16
        timings     40533 63 15 3 50 104 44
    EndMode
********************/

#if 0
static u8 registers_720_576[] = {
 0x00, 0x20, 0x00, 0x06, 0x00, 0xc0, 0x00, 0x50, 0x6c, 0x22, 0x71, 0x00, 0x53, 0x49, 0x00, 0x80,
 0x00, 0x00, 0xE8, 0x23, 0x84, 0x20, 0xcb, 0xac, 0x09, 0x2A, 0xFF, 0x10, 0x02, 0x80, 0x00, 0xaa,
 0x12, 0x0C, 0x71, 0x73, 0x44, 0x62, 0x34, 0x90, 0x4F, 0x5B, 0x15, 0x99, 0x22, 0x67, 0xFF, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x37,
 0x5F, 0xCF, 0x23, 0x70, 0x02, 0x5F, 0xBF, 0x7F, 0x23, 0x95, 0xD1, 0x28, 0x82, 0x06, 0x00, 0x00,
 0x04, 0x00, 0x00, 0x5F, 0x03, 0x82, 0x80, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#else
static u8 registers_720_576[] = {
 0x00, 0x20, 0x00, 0x06, 0x00, 0xC0, 0x00, 0xFF, 0x6E, 0x22, 0x82, 0x00, 0x51, 0x5C, 0x07, 0x8F,
 0x00, 0x00, 0xE8, 0x23, 0x84, 0x20, 0xCB, 0xAC, 0x09, 0x2A, 0xFF, 0x10, 0x02, 0x80, 0x00, 0xAA,
 0x17, 0x0C, 0x71, 0x79, 0x44, 0x62, 0x34, 0x90, 0x4F, 0x5B, 0x15, 0x99, 0x22, 0x67, 0xFF, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x58,
 0x5F, 0xCF, 0x23, 0x70, 0x02, 0x5F, 0xCC, 0x7F, 0x23, 0x93, 0xD0, 0xDD, 0x80, 0x06, 0x00, 0x00,
 0x04, 0x00, 0x00, 0x5F, 0x03, 0x82, 0x80, 0x81, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif


/*******************
720x480, 27.5MHz, NTSC no overscan compensation.
This mode should be use for digital video, DVD playback etc.

NOTE: This mode for NTSC, see 720x576 for an equivalent PAL mode

Compatible fbset modeline:

    Mode        "720x480-vt1622"
        geometry    720 480 720 480 16
        timings     40533 63 15 3 50 104 44
    EndMode
********************/
#if 0
static u8 registers_720_480[] = {
 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x20, 0x7d, 0x6a, 0xfe, 0x5a, 0x14, 0x4a, 0x41, 0x00, 0x80,
 0x00, 0x00, 0x1a, 0x23, 0xac, 0x69, 0xf8, 0xec, 0x04, 0x22, 0xee, 0x10, 0x00, 0x80, 0x00, 0x11,
 0x16, 0x08, 0x99, 0x6e, 0x10, 0x56, 0x31, 0x90, 0x51, 0x00, 0x00, 0xa3, 0x29, 0x5d, 0xc3, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x3c,
 0x57, 0xcf, 0x23, 0x0c, 0x02, 0x57, 0xed, 0x7f, 0x63, 0x92, 0xd6, 0xde, 0x6c, 0x06, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x57, 0x03, 0x73, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#else
static u8 registers_720_480[] = {
// 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x20, 0x79, 0x67, 0xFE, 0x7A, 0x14, 0x4E, 0x54, 0x07, 0x8F,
 0x00, 0x00, 0x00, 0x06, 0x03, 0x00, 0x20, 0x79, 0x67, 0xFE, 0x7A, 0x14, 0x4E, 0x54, 0x0f, 0x8F,
 0x00, 0x00, 0x1A, 0x23, 0xAC, 0x69, 0xF8, 0xED, 0x04, 0x22, 0xEE, 0x10, 0x00, 0x80, 0x00, 0x11,
 0x1D, 0x08, 0x99, 0x73, 0x10, 0x56, 0x31, 0x90, 0x51, 0x00, 0x00, 0xA3, 0x29, 0x5D, 0xC3, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x84, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x08, 0x51,
 0x57, 0xCF, 0x23, 0x0C, 0x02, 0x57, 0xEE, 0x7F, 0x63, 0x91, 0xD3, 0xDF, 0x6E, 0x06, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x57, 0x03, 0x73, 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
#endif

/*******************
640x480, NTSC overscan compensation.
This mode should be use for digital video, DVD playback etc.

Compatible fbset modeline:
    mode 640x480-ntsc
        geometry    640 480 640 480 16
        timings     40533 64 0 4 18 160 27
        hsync       high
        vsync       high
    endmode
********************/
static u8 registers_640_480[] = {
 0x00, 0x20, 0x03, 0x06, 0x03, 0x00, 0x20, 0x2f, 0x57, 0x08, 0x5e, 0x17, 0x50, 0x3f, 0x00, 0x80,
 0x00, 0x00, 0x70, 0x0d, 0x50, 0x38, 0x45, 0x99, 0x99, 0x29, 0xd0, 0x10, 0x02, 0x80, 0x00, 0x11,
 0x17, 0x0c, 0x48, 0x74, 0x08, 0x56, 0x2a, 0x90, 0x48, 0x00, 0x00, 0xa3, 0x29, 0x4e, 0xa7, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x3b,
 0x1f, 0x7f, 0x23, 0x0c, 0x02, 0xbb, 0x7f, 0x67, 0x22, 0x76, 0xb0, 0xa1, 0x2f, 0x05, 0x00, 0x69,
 0x80, 0x00, 0x00, 0x1f, 0x03, 0x50, 0x4d, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
};


int vt1622_id = 0;

static int vt1622_attach_adapter(struct i2c_adapter *adapter)
{
    return i2c_detect(adapter, &addr_data, vt1622_detect);
}

static struct i2c_client *vt1622_client = NULL;

void vt1622_dac_off(void)
{
    u8 out_sel = 0;
    u8 pwm = 0;

    if (vt1622_client == NULL)
        return;

    printk("vt1622_dac_off\n");

    out_sel = vt1622_read_value(vt1622_client, VT_OUTPUT_SELECT);
    pwm = vt1622_read_value(vt1622_client, VT_POWER_MANG);

    vt1622_write_value(vt1622_client, VT_OUTPUT_SELECT, out_sel & ~0x3);
    pwm |= 0xf; /* disable all DAC A, B, C, D */
    vt1622_write_value(vt1622_client, VT_POWER_MANG, pwm);
}


void vt1622_dac_svideo(void)
{
    u8 out_sel = 0;
    u8 pwm = 0;
    struct vt1622_data *data;

    if (vt1622_client == NULL)
        return;


    printk("vt1622_dac_svideo\n");

    out_sel = vt1622_read_value(vt1622_client, VT_OUTPUT_SELECT);
    pwm = vt1622_read_value(vt1622_client, VT_POWER_MANG);

    vt1622_write_value(vt1622_client, VT_OUTPUT_SELECT, out_sel & ~0x3);
    pwm |= 0xf; /* disable all DAC A, B, C, D */
    vt1622_write_value(vt1622_client, VT_POWER_MANG, pwm & ~0x06);  /* enable DAC B, C */
    data = vt1622_client->data;
    data->svideo = 2;   /* default YCbCr output */
}


/* This function is called by i2c_detect */
int vt1622_detect(struct i2c_adapter *adapter, int address,
         unsigned short flags, int kind)
{
    int i, cur;
    struct i2c_client *new_client = NULL;
    struct vt1622_data *data;
    int err = 0;

#define GPIO1           (1L << 1)

#define GPIO_BASE       0xb94c0000
#define GPIO_ENB        (GPIO_BASE + 0x54)
#define GPIO_DO         (GPIO_BASE + 0x56)  /* Data Ouput */
#define GPIO_OE         (GPIO_BASE + 0x58)  /* 1:Output/0:Input Selection */

writew(readw(GPIO_ENB) & ~(GPIO1), GPIO_ENB);
writew(readw(GPIO_OE) | GPIO1, GPIO_OE);
writew(0, GPIO_DO);

    printk("vt1622.o:  probing address %x .\n", address);
    /* Make sure we aren't probing the ISA bus!! This is just a safety check
       at this moment; i2c_detect really won't call us. */
#ifdef DEBUG
    if (i2c_is_isa_adapter(adapter)) {
        printk
            ("vt1622.o: vt1622_detect called for an ISA bus adapter?!?\n");
        return 0;
    }
#endif

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE |
                     I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
            goto ERROR;

    /* OK. For now, we presume we have a valid client. We now create the
       client structure, even though we cannot fill it completely yet.
       But it allows us to access vt1622_{read,write}_value. */
    if (!(new_client = kmalloc(sizeof(struct i2c_client) +
                   sizeof(struct vt1622_data),
                   GFP_KERNEL))) {
        err = -ENOMEM;
        goto ERROR;
    }

    data =
        (struct vt1622_data *) (((struct i2c_client *) new_client) + 1);
    new_client->addr = address;
    new_client->data = data;
    new_client->adapter = adapter;
    new_client->driver = &vt1622_driver;
    new_client->flags = 0;

    /* Now, we do the remaining detection. It is lousy. */
    cur = vt1622_read_value(new_client, VT_VID);
    if (cur != 0x10) {
        printk("vt1622.o: invalide version %x\n", cur);
        goto ERROR;
    }

    printk("vt1622.o: VT1622 A/AM detected (ver %x)\n",cur);

    /* Fill in the remaining client fields and put it into the global list */
    strcpy(new_client->name, "vt1622");

    new_client->id = vt1622_id++;
    data->valid = 0;
    init_MUTEX(&data->update_lock);

    /* Tell the I2C layer a new client has arrived */
    if ((err = i2c_attach_client(new_client)))
        goto ERROR;

    /* Register a new directory entry with module sensors */
    if ((i = i2c_register_entry(new_client, "vt1622",
                    vt1622_dir_table_template)) < 0) {
        err = i;
        goto ERROR;
    }
    data->sysctl_id = i;

    vt1622_init_client((struct i2c_client *) new_client);
    vt1622_client = new_client;
    return 0;

/* OK, this is not exactly good programming practice, usually. But it is
   very code-efficient in this case. */

ERROR:
    if (new_client) {
        i2c_detach_client(new_client);
        kfree(new_client);
    }

    return err;
}

static int vt1622_detach_client(struct i2c_client *client)
{
    int err;

    i2c_deregister_entry(((struct vt1622_data *) (client->data))->sysctl_id);

    if ((err = i2c_detach_client(client))) {
        printk("vt1622.o: Client deregistration failed, client not detached.\n");
        return err;
    }

    kfree(client);

    return 0;
}


/* All registers are byte-sized. */
static inline int vt1622_read_next(struct i2c_client *client)
{
    return i2c_smbus_read_byte(client);
}

static int vt1622_read_value(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}


/* All registers are byte-sized */
static int vt1622_write_value(struct i2c_client *client, u8 reg, u8 value)
{
    return i2c_smbus_write_byte_data(client, reg, value);
}

static void vt1622_write_values(struct i2c_client *client, const int nreg, u8 *values)
{
    int i;
    /* writes set of registers from array.  0,0 marks end of table */
    for (i = 0; i < nreg; i++)
        vt1622_write_value(client, i, values[i]);
}

static int default_vt_x = 720;
static int default_vt_y = 480;
static int default_vt_ntsc = 1;
static int default_vt_svideo = 2;

static void vt1622_init_client(struct i2c_client *client)
{
    struct vt1622_data *data = client->data;
    u8 *registers_set;

    data->res[0] = default_vt_x;
    data->res[1] = default_vt_y;
    data->ntsc = default_vt_ntsc;
    data->svideo = 3; default_vt_svideo;   /* default YCbCr output */
    data->colorbar = 0;
    data->laced = 0;    /* Interlaced */
    if (data->res[0] == 720 && data->res[1] == 480)
        registers_set = registers_720_480;
    else if (data->res[0] == 720 && data->res[1] == 576)
        registers_set = registers_720_576;
    else if (data->res[0] == 800 && data->res[1] == 480)
								registers_set = registers_800_480;
				else
        registers_set = registers_640_480;
    /* PAL */
    data->hvpos[0][0] = registers_720_576[VT_HPOS] | ((registers_720_576[VT_OVERFLOW] & 0x4) ? (1 << 8) : 0);
    data->hvpos[0][1] = registers_720_576[VT_VPOS] | ((registers_720_576[VT_OVERFLOW] & 0x2) ? (1 << 8) : 0);
    /* NTSC */
    data->hvpos[1][0] = registers_720_480[VT_HPOS] | ((registers_720_480[VT_OVERFLOW] & 0x4) ? (1 << 8) : 0);
    data->hvpos[1][1] = registers_720_480[VT_VPOS] | ((registers_720_480[VT_OVERFLOW] & 0x2) ? (1 << 8) : 0);

    vt1622_update_res(client);
    vt1622_update_client(client);
    vt1622_dac_off();
}



static void vt1622_update_client(struct i2c_client *client)
{
    struct vt1622_data *data = client->data;
    u8 cbar = 0;
    u8 overflow = 0;
    u8 out_sel = 0;
    u8 pwm = 0;
    int ntsc = data->ntsc;

    down(&data->update_lock);

    out_sel = vt1622_read_value(client, VT_OUTPUT_SELECT);
    pwm = vt1622_read_value(client, VT_POWER_MANG);
    /* Set composite/svideo mode, also enable the right dacs */
    if (data->svideo == 2) { /* YCbCr output */
        vt1622_write_value(client, VT_OUTPUT_SELECT, out_sel | 0x3);
        vt1622_write_value(client, VT_POWER_MANG, pwm & ~0x0f);  /* enable DAC A, B, C, D */
    } else if (data->svideo == 1) { /* SVIDEO Ouput */
        vt1622_write_value(client, VT_OUTPUT_SELECT, out_sel & ~0x3);
        pwm |= 0xf; /* disable all DAC A, B, C, D */
        vt1622_write_value(client, VT_POWER_MANG, pwm & ~0x06);  /* enable DAC B, C */
    } else if (data->svideo == 0) { /* Composited Output */
        vt1622_write_value(client, VT_OUTPUT_SELECT, out_sel & ~0x3);
        pwm |= 0xf; /* disable all DAC A, B, C, D */
        vt1622_write_value(client, VT_POWER_MANG, pwm & ~0x08);  /* enable DAC A */
    }

    /* Set Colorbar */
    cbar = vt1622_read_value(client, VT_LUMA_FILTER);
    if (data->colorbar) {
        vt1622_write_value(client, VT_LUMA_FILTER, cbar | 0x20);
    } else {
        vt1622_write_value(client, VT_LUMA_FILTER, cbar & ~0x20);
    }

    /* Set Output mode (laced) */
    out_sel = vt1622_read_value(client, VT_OUTPUT_MODE);
    out_sel &= ~0x0c;
    if (data->laced == 0) { /* interlaced */
        vt1622_write_value(client, VT_OUTPUT_MODE, out_sel);
    } else if (data->laced == 1) { /* non-interlaced */
        vt1622_write_value(client, VT_OUTPUT_MODE, out_sel | 0x4);
    } else {
        vt1622_write_value(client, VT_OUTPUT_MODE, out_sel | 0x8);
    }


    /* Set H/V Position */
    overflow = vt1622_read_value(client, VT_OVERFLOW);
    if ((data->hvpos[ntsc][0] & ((0x1) << 8)))
        overflow |= 0x4;
    else
        overflow &= ~0x4;
    if ((data->hvpos[ntsc][1] & ((0x1) << 8)))
        overflow |= 0x2;
    else
        overflow &= ~0x2;

    vt1622_write_value(client, VT_OVERFLOW, overflow);
    vt1622_write_value(client, VT_HPOS, data->hvpos[ntsc][0] & 0xff);
    vt1622_write_value(client, VT_VPOS, data->hvpos[ntsc][1] & 0xff);

    up(&data->update_lock);

}


void vt1622_update_res(struct i2c_client *client)
{
    struct vt1622_data *data = client->data;

    down(&data->update_lock);

    if (data->res[0] == 720 && data->res[1] == 480) {
        data->ntsc = 1;
        vt1622_write_values(client, sizeof(registers_720_480)/sizeof(registers_720_480[0]), registers_720_480);
    } else if (data->res[0] == 720 && data->res[1] == 576) {
        data->ntsc = 0;
        vt1622_write_values(client, sizeof(registers_720_576)/sizeof(registers_720_576[0]), registers_720_576);
    } else if (data->res[0] == 640 && data->res[1] == 480) {
        vt1622_write_values(client, sizeof(registers_640_480)/sizeof(registers_640_480[0]), registers_640_480);
    } else if (data->res[0] == 800 && data->res[1] == 480) {
        vt1622_write_values(client, sizeof(registers_800_480)/sizeof(registers_800_480[0]), registers_800_480);
    } else if (data->ntsc) {
        data->res[0] = 720; data->res[1] = 480;
        vt1622_write_values(client, sizeof(registers_720_480)/sizeof(registers_720_480[0]), registers_720_480);
    } else {
        data->res[0] = 720; data->res[1] = 576;
        vt1622_write_values(client, sizeof(registers_720_576)/sizeof(registers_720_576[0]), registers_720_576);
    }

    up(&data->update_lock);

}

void vt1622_res(struct i2c_client *client, int operation, int ctl_name,
           int *nrels_mag, long *results)
{
    struct vt1622_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        results[0] = data->res[0];
        results[1] = data->res[1];
        *nrels_mag = 2;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->res[0] = results[0];
        }
        if (*nrels_mag >= 2) {
            data->res[1] = results[1];
        }
        if (results[0] == 720 && results[1] == 480)
            data->ntsc = 1;
        else if (results[0] == 720 && results[1] == 576)
            data->ntsc = 0;
        vt1622_update_res(client);
        vt1622_update_client(client);
    }
}

void vt1622_ntsc(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct vt1622_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        results[0] = data->ntsc;
        *nrels_mag = 1;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->ntsc = (results[0] > 0);
        }
        vt1622_update_res(client);
        vt1622_update_client(client);
    }
}


void vt1622_svideo(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct vt1622_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        results[0] = data->svideo;
        *nrels_mag = 1;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->svideo = results[0];
        }
        vt1622_update_client(client);
    }
}

void vt1622_colorbar(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct vt1622_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        results[0] = data->colorbar;
        *nrels_mag = 1;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->colorbar = (results[0]) ? 1 : 0;
        }
        vt1622_update_client(client);
    }
}

void vt1622_hvpos(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct vt1622_data *data = client->data;
    int ntsc = data->ntsc;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        results[0] = data->hvpos[ntsc][0];
        results[1] = data->hvpos[ntsc][1];
        *nrels_mag = 2;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->hvpos[ntsc][0] = results[0] & 0x1ff;
        }
        if (*nrels_mag >= 2) {
            data->hvpos[ntsc][1] = results[1] & 0x1ff;
        }
        vt1622_update_client(client);
    }
}

void vt1622_laced(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct vt1622_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        results[0] = data->laced;
        *nrels_mag = 1;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->laced = results[0];
        }
        vt1622_update_client(client);
    }
}


static int __init sm_vt1622_init(void)
{
    int rc = 0;
    static int inited = 0;
    if (! inited) {
        rc = i2c_add_driver(&vt1622_driver);
        if (rc < 0) {
            printk("Failed to init vt1622 driver\n");
            goto EXIT;
        }
        printk("vt1622.o version %s (%s)\n", LM_VERSION, LM_DATE);
        inited = 1;
    }

    MOD_INC_USE_COUNT;

EXIT:
    return rc;
}

static void __exit sm_vt1622_exit(void)
{
    i2c_del_driver(&vt1622_driver);
    MOD_DEC_USE_COUNT;
}


#if 1
int __init vt1622_pixclock_init(void)
{
    return sm_vt1622_init();
}
#endif


/*
 * Syntax of vt1622 setup
 * vt1622=res:<xres>x<yres>[,ntsc|pal][,composite|svideo|component]
 * for example
 * vt1622=res:640x480,ntsc,component
 */

int __init vt1622_setup(char *options)
{

    if (strncmp(options, "res:",4) == 0) {
        options += 4;
	    if (*options) {
		    default_vt_x = simple_strtoul(options, &options, 0); /* xres */
    		if (*options != 'x')
	    		goto EXIT;
		    options++;
		    default_vt_y = simple_strtoul(options, &options, 0); /* yres */
        }
    }

    if (*options == ',')
        options++;

    if (strncmp(options, "ntsc", 4) == 0) {
        options+=5;
        default_vt_ntsc = 1;
    } else if (strncmp(options, "pal", 3) == 0) {
        options+=4;
        default_vt_ntsc = 0;
    }

    if (*options == ',')
        options++;

    if (strncmp(options, "composite", 9) == 0) {
        options+=9;
        default_vt_svideo = 0;
    } else if (strncmp(options, "svideo", 6) == 0) {
        options+=7;
        default_vt_svideo = 1;
    } else if (strncmp(options, "component", 9) == 0) {
        options+=9;
        default_vt_svideo = 2;
    }

EXIT:
    return 1;
}


/* #001 */
int __init vt1622_parameter_setup(char *options, u8 reg_tbl[])
{
    int idx, val;

    while(*options != '\0') {
        if (*options++ != '(')
            break;

        idx = simple_strtoul(options, &options, 16);

        if (*options++ != ',')
            break;

        val = simple_strtoul(options, &options, 16);

        if (*options++ != ')')
            break;

        reg_tbl[idx] = val;
    }

    return 1;
}

int __init vt1622_720_576_setup(char *options)
{
    return vt1622_parameter_setup(options, registers_720_576);
}

int __init vt1622_720_480_setup(char *options)
{
    return vt1622_parameter_setup(options, registers_720_480);
}

int __init vt1622_640_480_setup(char *options)
{
    return vt1622_parameter_setup(options, registers_640_480);
}


__setup("vt1622=", vt1622_setup);
__setup("vt1622_720_576=", vt1622_720_576_setup);
__setup("vt1622_720_480=", vt1622_720_480_setup);
__setup("vt1622_640_480=", vt1622_640_480_setup);


MODULE_AUTHOR("Jedy Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("vt1622 driver");
module_init(sm_vt1622_init);
module_exit(sm_vt1622_exit);
