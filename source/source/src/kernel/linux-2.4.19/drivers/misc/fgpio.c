/* GPIO Push Button driver by ivan wang */
#include <linux/module.h>
#include <linux/config.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <asm/signal.h>
#include <asm/arch/cpe/cpe.h>
#include <asm/arch/cpe_int.h>

static struct proc_dir_entry    *info_entry;
int                             play_c=0;
pid_t                           mpid;
struct file                     *filp;
spinlock_t                      fgpio_lock;

extern asmlinkage long sys_kill(int pid, int sig);

void fgpio_play_next(void)
{
    printk("Play Next:%d\n",mpid);
    /* send signal */
    sys_kill(mpid,SIGUSR1);
}

void fgpio_play_priv(void)
{
    printk("Play Priv:%d\n",mpid);
    /* send signal */
    sys_kill(mpid,SIGUSR2);
}

void gpio_interrupt_handler(int irq, void *dev_id, struct pt_regs *dummy)
{
    //determine which GPIO
    int             gpio_value;
    unsigned long   flags;
    
    spin_lock_irqsave(&fgpio_lock,flags);
    gpio_value=*(volatile unsigned int *)(CPE_GPIO_VA_BASE+0x24)&0x6;
    if(gpio_value==2)  //GPIO 1
        fgpio_play_priv();
    else if(gpio_value==4)  //GPIO 2
        fgpio_play_next();
    *(volatile unsigned int *)(CPE_GPIO_VA_BASE+0x30)=0x6; //clear interrupt

    spin_unlock_irqrestore(&fgpio_lock,flags);
}

static int fgpio_atoi(char *s)
{
    int k = 0;
    
    k = 0;
    while (*s != '\0' && *s >= '0' && *s <= '9') 
    {
        k = 10 * k + (*s - '0');
        s++;
    }
    return k;
}

static int proc_read_gpio_info(char *page, char **start,off_t off, int count,int *eof, void *data)
{
    int     len;
    len=sprintf(page,"%d\n",mpid);
    *eof=1; //end of file
    *start = page + off;
//printk("mpid=%d len:%d off:%d\n",mpid,len,off);
    len=len-off;
    return len;
}

static int proc_write_gpio_info(struct file *file,const char *buffer,unsigned long count, void *data)
{
    int len=count;
    unsigned char value[20];
    if(copy_from_user(value,buffer,len))
        return 0;
    value[len]='\0';
    mpid=fgpio_atoi(value);
//    printk("mpid=%d %s count=%d\n",mpid,value,count);
    return count;
}

static int __init fgpio_init(void)
{
    spin_lock_init(&fgpio_lock);

    /* create proc file system */
    //gpio_dir=proc_mkdir("gpio",NULL);

    /* info entry */
    info_entry=create_proc_entry("gpio",777,0);
    if(info_entry==NULL)
    {
        printk("Fail to create proc entry: info!\n");
        return 0;
    }
    info_entry->read_proc=proc_read_gpio_info;
    info_entry->write_proc=proc_write_gpio_info;
    info_entry->owner=THIS_MODULE;   
    mpid=0;

    *(volatile unsigned int *)(CPE_GPIO_VA_BASE+0x20)|=0x6;
    cpe_int_set_irq(IRQ_GPIO, EDGE, H_ACTIVE);
    if(request_irq(IRQ_GPIO, gpio_interrupt_handler, SA_INTERRUPT,"gpio",0) < 0)
        printk(KERN_WARNING "Faraday GPIO: Unable to allocate IRQ %d\n",IRQ_GPIO);
    return 0;
}

static void __exit fgpio_exit(void)
{
}

module_init(fgpio_init);
module_exit(fgpio_exit);
