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
#define LM_DATE "20041117"
#define LM_VERSION "1.0.1"

MODULE_LICENSE("GPL");

/* Addresses to scan */
static unsigned short normal_i2c[] = { SENSORS_I2C_END };
static unsigned short normal_i2c_range[] = {0x1a, 0x1a, SENSORS_I2C_END };
static unsigned int normal_isa[] = { SENSORS_ISA_END };
static unsigned int normal_isa_range[] = { SENSORS_ISA_END };

/* Insmod parameters */
SENSORS_INSMOD_1(pt5701);

struct pt5701_data {
    int sysctl_id;
    struct semaphore update_lock;
    char valid;         /* !=0 if following fields are valid */
};

static int pt5701_attach_adapter(struct i2c_adapter *adapter);
static int pt5701_detect(struct i2c_adapter *adapter, int address,
            unsigned short flags, int kind);
static int pt5701_detach_client(struct i2c_client *client);
int pt5701_read_value(u8 reg);
int pt5701_write_value(u8 reg, u8 value);

/* This is the driver that will be inserted */
static struct i2c_driver pt5701_driver = {
    .owner      = THIS_MODULE,
    .name       = "prolific audio codec driver",
    .id         = I2C_DRIVERID_PT5701,
    .flags      = I2C_DF_NOTIFY,
    .attach_adapter = pt5701_attach_adapter,
    .detach_client  = pt5701_detach_client,
};

static ctl_table pt5701_dir_table_template[] = {
    {0}
};


int pt5701_id = 0;

static int pt5701_attach_adapter(struct i2c_adapter *adapter)
{
    return i2c_detect(adapter, &addr_data, pt5701_detect);
}

static struct i2c_client *pt5701_client = NULL;


/* This function is called by i2c_detect */
int pt5701_detect(struct i2c_adapter *adapter, int address,
         unsigned short flags, int kind)
{
    int i, cur;
    struct i2c_client *new_client = NULL;
    struct pt5701_data *data;
    int err = 0;


    printk("pt5701.o:  probing address %x .\n", address);
    /* Make sure we aren't probing the ISA bus!! This is just a safety check
       at this moment; i2c_detect really won't call us. */
#ifdef DEBUG
    if (i2c_is_isa_adapter(adapter)) {
        printk
            ("pt5701.o: pt5701_detect called for an ISA bus adapter?!?\n");
        return 0;
    }
#endif

    if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_READ_BYTE |
                     I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
            goto ERROR;

    /* OK. For now, we presume we have a valid client. We now create the
       client structure, even though we cannot fill it completely yet.
       But it allows us to access pt5701_{read,write}_value. */
    if (!(new_client = kmalloc(sizeof(struct i2c_client) +
                   sizeof(struct pt5701_data),
                   GFP_KERNEL))) {
        err = -ENOMEM;
        goto ERROR;
    }

    data =
        (struct pt5701_data *) (((struct i2c_client *) new_client) + 1);
    new_client->addr = address;
    new_client->data = data;
    new_client->adapter = adapter;
    new_client->driver = &pt5701_driver;
    new_client->flags = 0;

#if 0
    /* Now, we do the remaining detection. It is lousy. */
    cur = pt5701_read_value(new_client, PT5701_VID);
    if (cur != 0x10) {
        printk("pt5701.o: invalide version %x\n", cur);
        goto ERROR;
    }

    printk("pt5701.o: Prolific PT5701 Audio Codec Detected (ver %x)\n",cur);
#endif

    /* Fill in the remaining client fields and put it into the global list */
    strcpy(new_client->name, "pt5701");

    new_client->id = pt5701_id++;
    data->valid = 0;
    init_MUTEX(&data->update_lock);

    /* Tell the I2C layer a new client has arrived */
    if ((err = i2c_attach_client(new_client)))
        goto ERROR;

    /* Register a new directory entry with module sensors */
    if ((i = i2c_register_entry(new_client, "pt5701",
                    pt5701_dir_table_template)) < 0) {
        err = i;
        goto ERROR;
    }
    data->sysctl_id = i;

    // pt5701_init_client((struct i2c_client *) new_client);
    pt5701_client = new_client;
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

static int pt5701_detach_client(struct i2c_client *client)
{
    int err;

    i2c_deregister_entry(((struct pt5701_data *) (client->data))->sysctl_id);

    if ((err = i2c_detach_client(client))) {
        printk("pt5701.o: Client deregistration failed, client not detached.\n");
        return err;
    }

    kfree(client);

    return 0;
}


int pt5701_read_value(u8 reg)
{
    if (pt5701_client)
        return i2c_smbus_read_byte_data(pt5701_client, reg);
    else
        return -1;
}


/* All registers are byte-sized */
int pt5701_write_value(u8 reg, u8 value)
{
    if (pt5701_client)
        return i2c_smbus_write_byte_data(pt5701_client, reg, value);
    else
        return -1;
}



int __init pt5701_init(void)
{
    int rc = 0;
    static int inited = 0;
    if (! inited) {
        rc = i2c_add_driver(&pt5701_driver);
        if (rc < 0) {
            printk("Failed to init pt5701 driver\n");
            goto EXIT;
        }
        printk("pt5701.o version %s (%s)\n", LM_VERSION, LM_DATE);
        inited = 1;
    }

    MOD_INC_USE_COUNT;

EXIT:
    return rc;
}

static void __exit pt5701_exit(void)
{
    i2c_del_driver(&pt5701_driver);
    MOD_DEC_USE_COUNT;
}


MODULE_AUTHOR("Jedy Wei <jedy-wei@prolific.com.tw>");
MODULE_DESCRIPTION("pt5701 driver");
module_init(pt5701_init);
module_exit(pt5701_exit);
