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
#define LM_DATE "20031120"
#define LM_VERSION "1.0.1"

MODULE_LICENSE("GPL");

/* Addresses to scan */
static unsigned short normal_i2c[] = {0x75, SENSORS_I2C_END };
static unsigned short normal_i2c_range[] = {SENSORS_I2C_END };
static unsigned int normal_isa[] = { SENSORS_ISA_END };
static unsigned int normal_isa_range[] = { SENSORS_ISA_END };

/* Insmod parameters */
SENSORS_INSMOD_1(ch700x);

/* Many ch700x constants specified below */
#define CH_DMR      0x00        /* Display Mode */
#define CH_FFR      0x01        /* Flicker Filter */
#define CH_VBW      0x03        /* Video Bandwidth */
#define CH_IDF      0x04        /* Input Data Format */
#define CH_CM       0x06        /* Clock Mode */
#define CH_SAV      0x07        /* Start Active Video */
#define CH_PO       0x08        /* Position Overflow */
#define CH_BLR      0x09        /* Black Level */
#define CH_HPR      0x0a        /* Horizontal Position */
#define CH_VPR      0x0b        /* Vertical Position */
#define CH_SPR      0x0d        /* Sync Polarity */
#define CH_PMR      0x0e        /* Power Management */
#define CH_CDR      0x10        /* Connection Detect */
#define CH_CE       0x11        /* Contrast Enhancement */
#define CH_MNE      0x13        /* PLL M and N extra bits */
#define CH_PLLM     0X14        /* PLL-M Value */
#define CH_PLLN     0x15        /* PLL-N Value */
#define CH_BCO      0x17        /* Buffered Clock */
#define CH_FSCI     0x18        /* Subcarrier Frequency Adjust */
#define CH_PLLC     0x20        /* PLL and Memory Control */
#define CH_CVIC     0x21        /* CIV Control */
#define CH_CIV      0x22        /* Calculated Fsc Increment Value */
#define CH_VID      0x25        /* Version ID */
#define CH_TR       0x26        /* Test */
#define CH_AR       0x3f        /* Address */

/* Each client has this additional data */
struct ch700x_data {
    int sysctl_id;

    struct semaphore update_lock;
    char valid;         /* !=0 if following fields are valid */
    unsigned long last_updated;     /* In jiffies */

    u8 status[3];       /* Register values */
    u16 res[2];         /* Resolution XxY */
    u8 ntsc;            /* 1=NTSC, 0=PAL */
    u8 svideo;          /* output format: (2=RGB) 1=SVIDEO, 0=Composite */
};

static int ch700x_attach_adapter(struct i2c_adapter *adapter);
static int ch700x_detect(struct i2c_adapter *adapter, int address,
            unsigned short flags, int kind);
static void ch700x_init_client(struct i2c_client *client);
static int ch700x_detach_client(struct i2c_client *client);
static inline int ch700x_read_next(struct i2c_client *client);
static int ch700x_read_value(struct i2c_client *client, u8 reg);
static int ch700x_write_value(struct i2c_client *client, u8 reg, u8 value);
static void ch700x_write_values(struct i2c_client *client, u8 *values);
static void ch700x_status(struct i2c_client *client, int operation,
               int ctl_name, int *nrels_mag, long *results);
static void ch700x_ntsc(struct i2c_client *client, int operation,
               int ctl_name, int *nrels_mag, long *results);
static void ch700x_res(struct i2c_client *client, int operation,
              int ctl_name, int *nrels_mag, long *results);
static void ch700x_svideo(struct i2c_client *client, int operation,
                int ctl_name, int *nrels_mag, long *results);
static void ch700x_update_client(struct i2c_client *client);


/* This is the driver that will be inserted */
static struct i2c_driver ch700x_driver = {
    .owner      = THIS_MODULE,
    .name       = "ch700x video-output chip driver",
    .id         = I2C_DRIVERID_CH700X,
    .flags      = I2C_DF_NOTIFY,
    .attach_adapter = ch700x_attach_adapter,
    .detach_client  = ch700x_detach_client,
};

/* -- SENSORS SYSCTL START -- */
#define CH700X_SYSCTL_STATUS 1000
#define CH700X_SYSCTL_NTSC   1001
#define CH700X_SYSCTL_HALF   1002
#define CH700X_SYSCTL_RES    1003
#define CH700X_SYSCTL_COLORBARS    1004
#define CH700X_SYSCTL_DEPTH  1005
#define CH700X_SYSCTL_SVIDEO 1006

/* -- SENSORS SYSCTL END -- */

/* These files are created for each detected ch700x. This is just a template;
   though at first sight, you might think we could use a statically
   allocated list, we need some way to get back to the parent - which
   is done through one of the 'extra' fields which are initialized
   when a new copy is allocated. */
static ctl_table ch700x_dir_table_template[] = {
    {CH700X_SYSCTL_STATUS, "status", NULL, 0, 0444, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &ch700x_status },
    {CH700X_SYSCTL_NTSC, "ntsc", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &ch700x_ntsc},
    {CH700X_SYSCTL_RES, "res", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &ch700x_res},
    {CH700X_SYSCTL_SVIDEO, "svideo", NULL, 0, 0644, NULL, &i2c_proc_real,
     &i2c_sysctl_real, NULL, &ch700x_svideo},
    {0}
};

/*******************
720x576, 27.5MHz, PAL, no overscan compensation.
This mode should be use for digital video, DVD playback etc.

NOTE: This mode for PAL, see 720x480 for an equivalent NTSC mode
NOTE:    -- Steve Davies <steve@daviesfam.org>

Compatible X modeline:

    Mode        "720x576-ch700x"
        DotClock        27.5
        HTimings        720 744 800 880
        VTimings        576 581 583 625
    EndMode
********************/

static u8 registers_720_576[] = {
    0, 0
};


/*******************
720x480, 27.5MHz, NTSC no overscan compensation.
This mode should be use for digital video, DVD playback etc.

NOTE: This mode for NTSC, see 720x576 for an equivalent PAL mode
NOTE:    -- Steve Davies <steve@daviesfam.org>

Compatible X modeline:

    Mode        "720x480-ch700x"
        DotClock        27.5
        HTimings        720 744 800 872
        VTimings        480 483 485 525
    EndMode
********************/
static u8 registers_720_480[] = {
    0, 0
};


int ch700x_id = 0;

static int ch700x_attach_adapter(struct i2c_adapter *adapter)
{
    return i2c_detect(adapter, &addr_data, ch700x_detect);
}

/* This function is called by i2c_detect */
int ch700x_detect(struct i2c_adapter *adapter, int address,
         unsigned short flags, int kind)
{
    int i, cur;
    struct i2c_client *new_client = NULL;
    struct ch700x_data *data;
    int err = 0;

    printk("ch700x.o:  probing address %x .\n", address);
    /* Make sure we aren't probing the ISA bus!! This is just a safety check
       at this moment; i2c_detect really won't call us. */
#ifdef DEBUG
    if (i2c_is_isa_adapter(adapter)) {
        printk
            ("ch700x.o: ch700x_detect called for an ISA bus adapter?!?\n");
        return 0;
    }
#endif

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE |
                     I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
            goto ERROR;

    /* OK. For now, we presume we have a valid client. We now create the
       client structure, even though we cannot fill it completely yet.
       But it allows us to access ch700x_{read,write}_value. */
    if (!(new_client = kmalloc(sizeof(struct i2c_client) +
                   sizeof(struct ch700x_data),
                   GFP_KERNEL))) {
        err = -ENOMEM;
        goto ERROR;
    }

    data =
        (struct ch700x_data *) (((struct i2c_client *) new_client) + 1);
    new_client->addr = address;
    new_client->data = data;
    new_client->adapter = adapter;
    new_client->driver = &ch700x_driver;
    new_client->flags = 0;

    /* Now, we do the remaining detection. It is lousy. */
    cur = ch700x_read_value(new_client, CH_VID);
    if (cur != 0x3a) {
        printk("ch700x.o: invalide version %x\n", cur);
        goto ERROR;
    }

    printk("ch700x.o: CH7005 detected (ver %x)\n",cur);

    /* Fill in the remaining client fields and put it into the global list */
    strcpy(new_client->name, "ch7005");

    new_client->id = ch700x_id++;
    data->valid = 0;
    init_MUTEX(&data->update_lock);

    /* Tell the I2C layer a new client has arrived */
    if ((err = i2c_attach_client(new_client)))
        goto ERROR;

    /* Register a new directory entry with module sensors */
    if ((i = i2c_register_entry(new_client, "ch700x",
                    ch700x_dir_table_template)) < 0) {
        err = i;
        goto ERROR;
    }
    data->sysctl_id = i;

    ch700x_init_client((struct i2c_client *) new_client);
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

static int ch700x_detach_client(struct i2c_client *client)
{
    int err;

    i2c_deregister_entry(((struct ch700x_data *) (client->data))->sysctl_id);

    if ((err = i2c_detach_client(client))) {
        printk("ch700x.o: Client deregistration failed, client not detached.\n");
        return err;
    }

    kfree(client);

    return 0;
}


/* All registers are byte-sized. */
static inline int ch700x_read_next(struct i2c_client *client)
{
    return i2c_smbus_read_byte(client);
}

static int ch700x_read_value(struct i2c_client *client, u8 reg)
{
    return i2c_smbus_read_byte_data(client, reg);
}


/* All registers are byte-sized */
static int ch700x_write_value(struct i2c_client *client, u8 reg, u8 value)
{
    return i2c_smbus_write_byte_data(client, reg, value);
}

static void ch700x_write_values(struct i2c_client *client, u8 *values)
{
    /* writes set of registers from array.  0,0 marks end of table */
    while (*values) {
        ch700x_write_value(client, values[0], values[1]);
        values += 2;
    }
}

static void ch700x_init_client(struct i2c_client *client)
{
    struct ch700x_data *data = client->data;

    /* Initialize the ch700x chip */
    ch700x_write_value(client, CH_PMR, 0x00);   /* Chip Reset */
    ch700x_write_value(client, CH_PMR, 0x0b);   /* Normal Power Mode */

    /* Choose Input Data Format */
    ch700x_write_value(client, CH_IDF, 0x00);   /* 16-bit non-multiplexed RGB 565 input */

    /* Choose default mode 16 */
    ch700x_write_value(client, CH_DMR, 0x69);   /* for mode 16 */
    /* Be a master mode to the clock */
    ch700x_write_value(client, CH_CM, 0x50);    /* Master Mode:1 MCP: 1*/
    // ch700x_write_value(client, CH_CM, 0x10);    /* Slave Mode:0 MCP: 1*/
    ch700x_write_value(client, CH_SAV, 11);
    /* Configure PLL */
    ch700x_write_value(client, CH_MNE, 0x00);
    ch700x_write_value(client, CH_PLLM, 0x3f);  /* PLL-M = 63 */
    ch700x_write_value(client, CH_PLLN, 0x6e);  /* PLL-N = 110 */

printk("CH_PMR = %x\n", ch700x_read_value(client, CH_PMR));
printk("CH_IDF = %x\n", ch700x_read_value(client, CH_IDF));
printk("CH_DMR = %x\n", ch700x_read_value(client, CH_DMR));
printk("CH_CM = %x\n", ch700x_read_value(client, CH_CM));
printk("CH_SAV = %x\n", ch700x_read_value(client, CH_SAV));
printk("CH_MNE = %x\n", ch700x_read_value(client, CH_MNE));
printk("CH_PLLM = %x\n", ch700x_read_value(client, CH_PLLM));
printk("CH_PLLN = %x\n", ch700x_read_value(client, CH_PLLN));

    data->res[0] = 640;
    data->res[1] = 480;
    data->ntsc = 1;
    data->svideo = 0;

}

static void ch700x_update_client(struct i2c_client *client)
{
    struct ch700x_data *data = client->data;
    down(&data->update_lock);
    if (! time_after_eq(data->last_updated + HZ + HZ/2, jiffies) &&  data->valid)
        goto EXIT;

    if ((data->res[0] == 800) && (data->res[1] == 600)) {
        /* 800x600 built-in mode */
        if (data->ntsc) {
            /* mode 22, 800x600 NTSC 5/6 */
        } else {
            /* mode 19, 800x600 PAL 1/1 */
        }
    }
    else if ((data->res[0] == 720) && (data->res[1] == 576)) {
            /* 720x576 no-overscan-compensation mode suitable for PAL DVD playback */
            data->ntsc = 0; /* This mode always PAL */
            ch700x_write_values(client,  registers_720_576);
    }
    else if ((data->res[0] == 720) && (data->res[1] == 480)) {
            /* 720x480 no-overscan-compensation mode suitable for NTSC DVD playback */
            data->ntsc = 1; /* This mode always NTSC */
            ch700x_write_values(client,  registers_720_480);
    }
    else {
        if (data->ntsc) {
            /* mode 16, 640x480 NTSC 1/1 */
            printk("ch700x.o:  change to mode 16, 640x480 NTSC 1/1\n");
    ch700x_write_value(client, CH_PMR, 0x00);   /* Chip Reset */
    ch700x_write_value(client, CH_PMR, 0x0b);   /* Normal Power Mode */

    /* Choose Input Data Format */
    ch700x_write_value(client, CH_IDF, 0x00);   /* 16-bit non-multiplexed RGB 565 input */

    /* Choose default mode 16 */
    ch700x_write_value(client, CH_DMR, 0x69);   /* for mode 16 */
    /* Be a master mode to the clock */
    ch700x_write_value(client, CH_CM, 0x50);    /* Master Mode:1 MCP: 1*/
    // ch700x_write_value(client, CH_CM, 0x10);    /* Slave Mode:0 MCP: 1*/
    ch700x_write_value(client, CH_SAV, 11);
    /* Configure PLL */
    ch700x_write_value(client, CH_MNE, 0x00);
    ch700x_write_value(client, CH_PLLM, 0x3f);  /* PLL-M = 63 */
    ch700x_write_value(client, CH_PLLN, 0x6e);  /* PLL-N = 110 */

        } else {
            /* mode 14, 640x480 PAL 1/1 */
        }

        if ((data->res[0] != 640) || (data->res[1] != 480)) {
            printk("ch700x.o:  Warning: arbitrary resolutions not supported yet.  Using 640x480.\n");
            data->res[0] = 640;
            data->res[1] = 480;
        }
    }

    /* Set composite/svideo mode, also enable the right dacs */
    if (data->svideo) {
        /* SVIDEO Output*/
    } else {
        /* Composite Output */
    }

EXIT:
    up(&data->update_lock);

}


void ch700x_status(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    u8 val;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        val = ch700x_read_value(client, CH_PMR);
        ch700x_write_value(client, CH_PMR, 0x0b);   /* Normal Power Mode */
        ch700x_write_value(client, CH_CDR, 0x01);   /* Turn on connection sensor */
        results[0] = ch700x_read_value(client, CH_CDR) & 0x0f;
        ch700x_write_value(client, CH_PMR, val);
        *nrels_mag = 1;
    }
}


void ch700x_ntsc(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct ch700x_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        ch700x_update_client(client);
        results[0] = data->ntsc;
        *nrels_mag = 1;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->ntsc = (results[0] > 0);
        }
        ch700x_update_client(client);
    }
}


void ch700x_svideo(struct i2c_client *client, int operation, int ctl_name,
        int *nrels_mag, long *results)
{
    struct ch700x_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        ch700x_update_client(client);
        results[0] = data->svideo;
        *nrels_mag = 1;
    } else if (operation == SENSORS_PROC_REAL_WRITE) {
        if (*nrels_mag >= 1) {
            data->svideo = (results[0]) ? 1 : 0;
        }
        ch700x_update_client(client);
    }
}


void ch700x_res(struct i2c_client *client, int operation, int ctl_name,
           int *nrels_mag, long *results)
{
    struct ch700x_data *data = client->data;
    if (operation == SENSORS_PROC_REAL_INFO)
        *nrels_mag = 0;
    else if (operation == SENSORS_PROC_REAL_READ) {
        ch700x_update_client(client);
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
        ch700x_update_client(client);
    }
}

static int __init sm_ch700x_init(void)
{
    printk("ch700x.o version %s (%s)\n", LM_VERSION, LM_DATE);
    return i2c_add_driver(&ch700x_driver);
}

static void __exit sm_ch700x_exit(void)
{
    i2c_del_driver(&ch700x_driver);
}



MODULE_AUTHOR("Jedy Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("ch700x driver");
module_init(sm_ch700x_init);
module_exit(sm_ch700x_exit);
