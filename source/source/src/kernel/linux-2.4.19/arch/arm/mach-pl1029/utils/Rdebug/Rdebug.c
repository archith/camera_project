#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h>     /* everything... */
#include <linux/errno.h>  /* error codes */
#include <linux/types.h>  /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <linux/pci.h>        /* O_ACCMODE */
#include <asm/system.h>   /* cli(), *_flags */
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/delay.h>
#include <linux/init.h>					/* module_init/module_exit */   
#include <linux/vmalloc.h>
#include "Rdebug.h"        /* local definitions */


#ifdef CONFIG_DEVFS_FS
devfs_handle_t regdebug_devfs_dir;
static char devname[4]="reg";
#endif


int regdebug_major =   REGDEBUG_MAJOR;
int regdebug_devs =    REGDEBUG_DEVS;    /* number of bare regdebug devices */
int regdebug_address = 0;

typedef struct reg_test_s{
  unsigned int target_addr;
  unsigned int register_value;
} reg_test_t;



Regdebug_Dev *regdebug_devices; /* allocated in regdebug_init */

int read_register_virt_w(unsigned int reg_virt_addr)
{ 
    printk("Unit:Word -- read register value at virt address %#010x ====> value = %#010x\n",reg_virt_addr,readl(reg_virt_addr));
    return 0;
}
int read_register_phy_w(unsigned int reg_phy_addr)
{
    printk("Unit:Word -- read register value at phy address %#010x ====> value = %#010x\n",reg_phy_addr,readl((u32)VIRT_IO_ADDRESS(reg_phy_addr)));
    return 0;
}

int write_register_virt_w(unsigned int reg_virt_addr ,unsigned int regvalue)
{
    writel(regvalue,reg_virt_addr);
    printk("Unit:Word -- write register at virt address %#010x ====> value = %#010x\n",reg_virt_addr,regvalue);
    return 0;
}

int write_register_phy_w(unsigned int reg_phy_addr ,unsigned int regvalue)
{
    writel(regvalue,(u32)VIRT_IO_ADDRESS(reg_phy_addr));
    printk("Unit:Word -- write register at phy address %#010x ====> value = %#010x\n",reg_phy_addr,regvalue);
    return 0;
}



int read_user_virt_w(unsigned int user_virt_addr)
{
    printk("Unit:Word -- read user memory space value at virt address %#010x ====> value = %#010x\n",user_virt_addr,readl(user_virt_addr));
    return 0;
}
int read_user_phy_w(unsigned int user_phy_addr)
{
    printk("Unit:Word -- read user memory space value at phy address %#010x ====> value = %#010x\n",user_phy_addr,readl((u32)__phys_to_virt(user_phy_addr)));
    return 0;
}

int write_user_virt_w(unsigned int user_virt_addr ,unsigned int value)
{
    writel(value,user_virt_addr);
    printk("Unit:Word -- write user memory space at virt address %#010x ====> value = %#010x\n",user_virt_addr,value);
    return 0;
}

int write_user_phy_w(unsigned int user_phy_addr ,unsigned int value)
{
    writel(value,(u32)__phys_to_virt(user_phy_addr));
    printk("Unit:Word -- write user memory space at phy address %#010x ====> value = %#010x\n",user_phy_addr,value);
    return 0;
}

/**********************************************************************************************************************************/

int read_register_virt_b(unsigned int reg_virt_addr)
{
    printk("Unit:Byte read register value at virt address %#010x ====> value = %#04x\n",reg_virt_addr,readb(reg_virt_addr));
    return 0;
}
int read_register_phy_b(unsigned int reg_phy_addr)
{
    printk("Unit:Byte -- read register value at phy address %#010x ====> value = %#04x\n",reg_phy_addr,readb((u32)VIRT_IO_ADDRESS(reg_phy_addr)));
    return 0;
}

int write_register_virt_b(unsigned int reg_virt_addr ,unsigned int regvalue)
{
    regvalue= (readl(reg_virt_addr)&0xffffff00)|(regvalue&0x000000ff);
    writel(regvalue,reg_virt_addr);
//    writeb(regvalue,reg_virt_addr);
    printk("Unit:Byte -- write register at virt address %#010x ====> value = %#04x\n",reg_virt_addr,regvalue);
    return 0;
}

int write_register_phy_b(unsigned int reg_phy_addr ,unsigned int regvalue)
{
    regvalue= (readl((u32)VIRT_IO_ADDRESS(reg_phy_addr))&0xffffff00)|(regvalue&0x000000ff);
    writel(regvalue,(u32)VIRT_IO_ADDRESS(reg_phy_addr));
//    writeb(regvalue,(u32)VIRT_IO_ADDRESS(reg_phy_addr));
    printk("Unit:Byte -- write register at phy address %#010x ====> value = %#04x\n",reg_phy_addr,regvalue);
    return 0;
}



int read_user_virt_b(unsigned int user_virt_addr)
{
    printk("Unit:Byte -- read user memory space value at virt address %#010x ====> value = %#04x\n",user_virt_addr,readb(user_virt_addr));
    return 0;
}
int read_user_phy_b(unsigned int user_phy_addr)
{
    printk("Unit:Byte -- read user memory space value at phy address %#010x ====> value = %#04x\n",user_phy_addr,readb((u32)__phys_to_virt(user_phy_addr)));
    return 0;
}

int write_user_virt_b(unsigned int user_virt_addr ,unsigned int value)
{
    writeb(value,user_virt_addr);
    printk("Unit:Byte -- write user memory space at virt address %#010x ====> value = %#04x\n",user_virt_addr,value);
    return 0;
}

int write_user_phy_b(unsigned int user_phy_addr ,unsigned int value)
{
    writeb(value,(u32)__phys_to_virt(user_phy_addr));
    printk("Unit:Byte -- write user memory space at phy address %#010x ====> value = %#04x\n",user_phy_addr,value);
    return 0;
}




/*
 * Open and close
 */

int regdebug_open (struct inode *inode, struct file *filp)
{
    int num = MINOR(inode->i_rdev);
    Regdebug_Dev *dev; /* device information */

    /*  check the device number */
    if (num >= regdebug_devs) return -ENODEV;
    dev = &regdebug_devices[num];

    /* now trim to 0 the length of the device if open was write-only */
     if ( (filp->f_flags & O_ACCMODE) == O_WRONLY) {
        if (down_interruptible (&dev->sem))
            return -ERESTARTSYS;
        regdebug_trim(dev); /* ignore errors */
        up (&dev->sem);
    }

    /* and use filp->private_data to point to the device data */
    filp->private_data = dev;

    return 0;          /* success */
}

int regdebug_release (struct inode *inode, struct file *filp)
{
    return 0;
}


/*
 * Data management: read and write
 */

ssize_t regdebug_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{    
    ssize_t retval = 0;
    return retval;
}



ssize_t regdebug_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval = 0;   
    return retval;
}

/*
 * The ioctl() implementation
 */
int regdebug_ioctl (struct inode *inode, struct file *filp,unsigned int cmd, unsigned long arg)
                 
{
    int datasize=0;
    reg_test_t *reg_data;
    unsigned int target_addr;
    unsigned int register_value;

    datasize=sizeof(struct reg_test_s);
    reg_data = kmalloc(datasize, GFP_KERNEL);
    if (reg_data == NULL) return -ENOMEM;
    memset(reg_data, 0, datasize);
    if (copy_from_user(reg_data, (void *) arg, datasize)) {
            kfree(reg_data);
            return -EFAULT;
    }

    target_addr=(unsigned int)reg_data->target_addr;
    register_value=(unsigned int)reg_data->register_value;

    /* don't even decode wrong cmds: better returning  ENOTTY than EFAULT */
    if (_IOC_TYPE(cmd) != REGDEBUG_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > REGDEBUG_IOC_MAXNR) return -ENOTTY;

    switch(cmd) {
                                                                      
      case REGDEBUG_REG_READ_VIRT_B:     
                                  read_register_virt_b(target_addr);
                                  break;       

      case REGDEBUG_REG_READ_PHY_B:
                                  read_register_phy_b(target_addr);
                                  break;

      case REGDEBUG_REG_READ_VIRT_W:
                                  read_register_virt_w(target_addr);
                                  break;

      case REGDEBUG_REG_READ_PHY_W:
                                  read_register_phy_w(target_addr);
                                  break;

      case REGDEBUG_REG_WRITE_VIRT_B:
                                  write_register_virt_b(target_addr,register_value);
                                  break;

      case REGDEBUG_REG_WRITE_PHY_B:
                                  write_register_phy_b(target_addr,register_value);
                                  break;

      case REGDEBUG_REG_WRITE_VIRT_W:
                                  write_register_virt_w(target_addr,register_value);
                                  break;

      case REGDEBUG_REG_WRITE_PHY_W:
                                  write_register_phy_w(target_addr,register_value);
                                  break;
                                  
      case REGDEBUG_USER_READ_VIRT_B:
                                  read_user_virt_b(target_addr);
                                  break;

      case REGDEBUG_USER_READ_PHY_B:
                                  read_user_phy_b(target_addr);
                                  break;

      case REGDEBUG_USER_READ_VIRT_W:
                                  read_user_virt_w(target_addr);
                                  break;

      case REGDEBUG_USER_READ_PHY_W:
                                  read_user_phy_w(target_addr);
                                  break;

      case REGDEBUG_USER_WRITE_VIRT_B:
                                  write_user_virt_b(target_addr,register_value);
                                  break;

      case REGDEBUG_USER_WRITE_PHY_B:
                                  write_user_phy_b(target_addr,register_value);
                                  break;
                                  
      case REGDEBUG_USER_WRITE_VIRT_W:
                                  write_user_virt_w(target_addr,register_value);
                                  break;

      case REGDEBUG_USER_WRITE_PHY_W:
                                  write_user_phy_w(target_addr,register_value);
                                  break;
        
      default:  
                                  return -ENOTTY;
    }
    return 1;

}


/*
 * The fops
 */

struct file_operations regdebug_fops = {
    read: regdebug_read,
    write: regdebug_write,
    ioctl: regdebug_ioctl,
    open: regdebug_open,
    release: regdebug_release,
};


int regdebug_trim(Regdebug_Dev *dev)
{   
    return 0;
}




/*
 * Finally, the module stuff
 */

static int __init regdebug_init(void)
{
    int result;

    SET_MODULE_OWNER(&regdebug_fops);

#ifdef CONFIG_DEVFS_FS
    regdebug_devfs_dir = devfs_mk_dir(NULL, "Rdebug", NULL);
    if (!regdebug_devfs_dir) return -EBUSY; // problem 
#else // no devfs, do it the "classic" way        
    /*
     * Register your major, and accept a dynamic number
     */
    result = register_chrdev(regdebug_major, "Rdebug", &regdebug_fops);
    if (result < 0) return result;
    if (regdebug_major == 0) regdebug_major = result; /* dynamic */
#endif // CONFIG_DEVFS_FS 
    
    /* 
     * allocate the devices -- we can't have them static, as the number
     * can be specified at load time
     */
    regdebug_devices = kmalloc(regdebug_devs * sizeof (Regdebug_Dev), GFP_KERNEL);
    if (!regdebug_devices) {
        result = -ENOMEM;
        goto fail_malloc;
    }
    memset(regdebug_devices, 0, regdebug_devs * sizeof (Regdebug_Dev));
    fail_malloc:
    unregister_chrdev(regdebug_major, "Rdebug");
#ifdef CONFIG_DEVFS_FS
         devfs_register(regdebug_devfs_dir, devname,
                       DEVFS_FL_AUTO_DEVNUM,
                       0, 0, S_IFCHR | S_IRUGO | S_IWUGO,
                       &regdebug_fops,
                       regdebug_devices);
#endif
    return 0; /* succeed */
}

void regdebug_cleanup(void)
{
    int i;
    unregister_chrdev(regdebug_major, "Rdebug");
    if(regdebug_devices){
      for (i=0; i<regdebug_devs; i++){
         regdebug_trim(regdebug_devices+i);
//         devfs_unregister(scull_devices[i].handle);
      }        
      kfree(regdebug_devices);
    }
    devfs_unregister(regdebug_devfs_dir);
}

module_init(regdebug_init);
module_exit(regdebug_cleanup);
