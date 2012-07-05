
/*
 * driver configuration section.  Here are the various options:
 *
 */

#include <linux/config.h>
#include <linux/version.h>

#ifdef MODULE
#include <linux/module.h>
#endif

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/ptrace.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <asm/system.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/bitops.h>
#include <linux/random.h>
#include <asm/arch-cpe/irq.h>
#include <asm/uaccess.h>

#include <asm/arch/cpe/cpe.h>   //All register definition
#include <asm/arch/cpe_int.h>   //Interrupt definition

#define REGS_SIZE  			0x6C	

/* register */
	
#define SEC_EncryptControl		0x00000000	
#define SEC_SlaveRunEnable		0x00000004
#define SEC_FIFOStatus			0x00000008
#define SEC_PErrStatus			0x0000000C
#define SEC_DESKey1H			0x00000010


#define SEC_DESKey1L			0x00000014	
#define SEC_DESKey2H			0x00000018
#define SEC_DESKey2L			0x0000001C
#define SEC_DESKey3H			0x00000020
#define SEC_DESKey3L			0x00000024
#define SEC_AESKey6				0x00000028
#define SEC_AESKey7				0x0000002C
#define SEC_DESIVH				0x00000030

#define SEC_DESIVL				0x00000034
#define SEC_AESIV2				0x00000038
#define SEC_AESIV3				0x0000003C
#define SEC_INFIFOPort			0x00000040
#define SEC_OutFIFOPort			0x00000044
#define SEC_DMASrc				0x00000048
#define SEC_DMADes				0x0000004C	

#define SEC_DMATrasSize			0x00000050
#define SEC_DMACtrl				0x00000054
#define SEC_FIFOThreshold		0x00000058
#define SEC_IntrEnable			0x0000005C
#define SEC_IntrStatus			0x00000060
#define SEC_MaskedIntrStatus	0x00000064
#define SEC_ClearIntrStatus		0x00000068

#define SEC_LAST_IV0			0x00000080
#define SEC_LAST_IV1			0x00000084
#define SEC_LAST_IV2			0x00000088
#define SEC_LAST_IV3			0x0000008c

/* bit mapping of command register */
//SEC_EncryptControl
#define Parity_check			0x100
#define First_block				0x80
#define ECB_mode				0x00
#define CBC_mode				0x10
#define CTR_mode				0x20
#define CFB_mode				0x40
#define OFB_mode				0x50
#define Algorithm_DES			0x0
#define Algorithm_Triple_DES	0x2
#define Algorithm_AES_128		0x8
#define Algorithm_AES_192		0xA
#define Algorithm_AES_256		0xC

//SEC_DMACtrl
#define DMA_Enable				0x1

//SEC_IntrStatus
#define Data_done				0x1

#define Decrypt_Stage			0x1
#define Encrypt_Stage			0x0

//#define DEBUG

typedef struct es_private
{	
	void			*regp;
	int 			irq;
	struct timer_list 	watchdog_timer;	/* watchdog timer id */
    	
}es_private; 

typedef struct esreq
{	
	int		algorithm;
	int		mode;	
	
	u32		data_length;
	u32		key_length;	
	
	u32		key_addr[8];
	u32		IV_addr[4];	
	
	u32		callin_addr;//application to driver, mapping to VM_DataIn_addr
	u32		return_addr;//driver to application, mapping to VM_DataOut_addr
	
	u32		DataIn_addr;	
	u32		DataOut_addr;	
	
}esreq;

/* Use 'f' as magic number */
#define IOC_MAGIC  'f'

#define ES_GETKEY             	_IOWR(IOC_MAGIC, 8, esreq)
#define ES_ENCRYPT            	_IOWR(IOC_MAGIC, 9, esreq)
#define ES_DECRYPT             	_IOWR(IOC_MAGIC, 10, esreq)
#define ES_AUTO_ENCRYPT    		_IOWR(IOC_MAGIC, 11, esreq)
#define ES_AUTO_DECRYPT    		_IOWR(IOC_MAGIC, 12, esreq)
 
struct es_private *macp;	
	
static int es_open(struct inode * inode, struct file * filp);	
static int es_ioctl(struct inode * inode, struct file *filp, unsigned int cmd, unsigned long arg);
static int es_close(struct inode * inode, struct file * filp);

static int getkey(int algorithm, struct esreq *srq);
void IV_Output(u32 addr);
static int ES_DMA(int stage, int algorithm, int mode, struct esreq *srq, u32 Tmp_DataIn_addr, u32 Tmp_DataOut_addr, long Tmp_data_length);

volatile int DMA_INT_OK;
u32 rand;
  
/*
 * End of driver configuration section.
 */

inline void ES_WriteReg(int offset, u32 value)
{
	writel(value, macp->regp + offset);
}

inline u32 ES_ReadReg(int offset)
{
	return readl(macp->regp + offset);
}
 
static int es_open(struct inode * inode, struct file * filp)
{
	//printk("es_open\n");		
	return 0;
} 

 
static int es_close(struct inode * inode, struct file * filp)
{	
	//printk("es_close\n");	
	return 0;
}


/*
 * This is the interrupt routine
 */
static void es_interrupt(void)
{
	int status;

	//status = readl(macp->regp + SEC_MaskedIntrStatus);
	status = ES_ReadReg(SEC_MaskedIntrStatus);
	//printk("status = 0x%08x\n", status);

	if ((status & Data_done) != 0)
	{
		DMA_INT_OK = 1;	
		ES_WriteReg(SEC_ClearIntrStatus, Data_done);
		//printk("Int DMA_INT_OK = 0x%x\n",DMA_INT_OK);
	}
}
 

static int es_ioctl(struct inode * inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct esreq srq;
	//unsigned long flags;
	int err = 0; //i
	//u32 Tmp_DataIn_addr, Tmp_DataOut_addr;
	//long Tmp_data_length;
	u32 VM_DataIn_addr, VM_DataOut_addr, Phy_DataIn_addr, Phy_DataOut_addr;

	srq = *(esreq *)arg;

	//printk("es_ioctl\n"); 
	
	(u32)VM_DataIn_addr = consistent_alloc( GFP_DMA|GFP_KERNEL, srq.data_length, &(Phy_DataIn_addr));
	(u32)VM_DataOut_addr = consistent_alloc( GFP_DMA|GFP_KERNEL, srq.data_length, &(Phy_DataOut_addr));	
	
	//printk("VM mapping address = 0x%08x 0x%08x\n", VM_DataIn_addr, VM_DataOut_addr);
		
	switch (cmd){
				
//==============================================================================
//Security_genkey	
//==============================================================================				
		case ES_GETKEY:		
			//printk("Security_genkey\n");
			
			err = getkey(srq.algorithm, &srq);
			if(err != 0)
				return err;
				
			break;
		
//==============================================================================
//Security_encrypt		
//==============================================================================
		case ES_ENCRYPT:
			printk("Security_encrypt\n");
			
			if (copy_from_user((u32 *)VM_DataIn_addr, (u32 *)srq.callin_addr, srq.data_length))
			{
				printk("error to copy_from_user\n");	
				return -1;	
			}	
				
//printk("driver In  = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataIn_addr, *(u32 *)(VM_DataIn_addr+4), *(u32 *)(VM_DataIn_addr+8), *(u32 *)(VM_DataIn_addr+12), *(u32 *)(VM_DataIn_addr+16), *(u32 *)(VM_DataIn_addr+20)); 	
/*			
*(u32 *)VM_DataIn_addr			=0x00000080;
*(u32 *)(VM_DataIn_addr + 4)	=0x00000000;
*(u32 *)(VM_DataIn_addr + 8)	=0x00000040;
*(u32 *)(VM_DataIn_addr + 12)	=0x00000000;
*(u32 *)(VM_DataIn_addr + 16)	=0x00000020;
*(u32 *)(VM_DataIn_addr + 20)	=0x00000000;
*/
				
			err = ES_DMA(Encrypt_Stage, srq.algorithm, srq.mode, &srq, Phy_DataIn_addr, Phy_DataOut_addr, srq.data_length);
			if(err != 0)
				return err;	
			IV_Output(VM_DataOut_addr + srq.data_length - 16);
						
			if (copy_to_user((u32 *)srq.return_addr, (u32 *)VM_DataOut_addr, srq.data_length))
			{
				printk("error to copy_to_user\n");
				return -1;
			}	

//printk("driver Out = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataOut_addr, *(u32 *)(VM_DataOut_addr+4), *(u32 *)(VM_DataOut_addr+8), *(u32 *)(VM_DataOut_addr+12), *(u32 *)(VM_DataOut_addr+16), *(u32 *)(VM_DataOut_addr+20)); 	
						
			break;
			
//==============================================================================
//Security_decrypt	
//==============================================================================		
		case ES_DECRYPT:
			printk("Security_decrypt\n");
			
			if (copy_from_user((u32 *)VM_DataIn_addr, (u32 *)srq.callin_addr, srq.data_length))
			{
				printk("error to copy_from_user\n");	
				return -1;	
			}

//printk("driver In  = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataIn_addr, *(u32 *)(VM_DataIn_addr+4), *(u32 *)(VM_DataIn_addr+8), *(u32 *)(VM_DataIn_addr+12), *(u32 *)(VM_DataIn_addr+16), *(u32 *)(VM_DataIn_addr+20)); 	
								
			err = ES_DMA(Encrypt_Stage, srq.algorithm, srq.mode, &srq, Phy_DataIn_addr, Phy_DataOut_addr, srq.data_length);
			if(err != 0)
				return err;	
			IV_Output(VM_DataOut_addr + srq.data_length - 16);
							
			if (copy_to_user((u32 *)srq.return_addr, (u32 *)VM_DataOut_addr, srq.data_length))
			{
				printk("error to copy_to_user\n");
				return -1;
			}	

//printk("driver Out = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataOut_addr, *(u32 *)(VM_DataOut_addr+4), *(u32 *)(VM_DataOut_addr+8), *(u32 *)(VM_DataOut_addr+12), *(u32 *)(VM_DataOut_addr+16), *(u32 *)(VM_DataOut_addr+20)); 	
								
			break;
				
//==============================================================================
//Security_autoencrypt	
//==============================================================================			
		case ES_AUTO_ENCRYPT:
			printk("Security_autoencrypt\n");
			
			err = getkey(srq.algorithm, &srq);
			if(err != 0)
				return err;
/*				
srq.IV_addr[0] = 0x00000000; 	
srq.key_addr[0] = 0x01010101; 		
srq.key_addr[1] = 0x01010101; 		
*/								
			if (copy_from_user((u32 *)VM_DataIn_addr, (u32 *)srq.callin_addr, srq.data_length))
			{
				printk("error to copy_from_user\n");	
				return -1;	
			}
			
//printk("driver In  = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataIn_addr, *(u32 *)(VM_DataIn_addr+4), *(u32 *)(VM_DataIn_addr+8), *(u32 *)(VM_DataIn_addr+12), *(u32 *)(VM_DataIn_addr+16), *(u32 *)(VM_DataIn_addr+20)); 	
										
			err = ES_DMA(Encrypt_Stage, srq.algorithm, srq.mode, &srq, Phy_DataIn_addr, Phy_DataOut_addr, srq.data_length);
			if(err != 0)
				return err;	
			IV_Output(VM_DataOut_addr + srq.data_length - 16);				
										
			if (copy_to_user((u32 *)srq.return_addr, (u32 *)VM_DataOut_addr, srq.data_length))
			{
				printk("error to copy_to_user\n");
				return -1;
			}	

//printk("driver Out = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataOut_addr, *(u32 *)(VM_DataOut_addr+4), *(u32 *)(VM_DataOut_addr+8), *(u32 *)(VM_DataOut_addr+12), *(u32 *)(VM_DataOut_addr+16), *(u32 *)(VM_DataOut_addr+20)); 	
										
			break;
			
//==============================================================================
//Security_autodecrypt	
//==============================================================================		
		case ES_AUTO_DECRYPT:
			printk("Security_autodecrypt\n");	
		
			err = getkey(srq.algorithm, &srq);
			if(err != 0)
				return err;						
							
			if (copy_from_user((u32 *)VM_DataIn_addr, (u32 *)srq.callin_addr, srq.data_length))
			{
				printk("error to copy_from_user\n");	
				return -1;	
			}
			
//printk("driver In  = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataIn_addr, *(u32 *)(VM_DataIn_addr+4), *(u32 *)(VM_DataIn_addr+8), *(u32 *)(VM_DataIn_addr+12), *(u32 *)(VM_DataIn_addr+16), *(u32 *)(VM_DataIn_addr+20)); 	
			
			err = ES_DMA(Encrypt_Stage, srq.algorithm, srq.mode, &srq, Phy_DataIn_addr, Phy_DataOut_addr, srq.data_length);
			if(err != 0)
				return err;	
			IV_Output(VM_DataOut_addr + srq.data_length - 16);
							
			if (copy_to_user((u32 *)srq.return_addr, (u32 *)VM_DataOut_addr, srq.data_length))
			{
				printk("error to copy_to_user\n");
				return -1;
			}	

//printk("driver Out = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataOut_addr, *(u32 *)(VM_DataOut_addr+4), *(u32 *)(VM_DataOut_addr+8), *(u32 *)(VM_DataOut_addr+12), *(u32 *)(VM_DataOut_addr+16), *(u32 *)(VM_DataOut_addr+20)); 	
			
			break;

		default:
			return -ENOIOCTLCMD;
	}
		
	consistent_free((u32 *)VM_DataIn_addr, srq.data_length, Phy_DataIn_addr);
	consistent_free((u32 *)VM_DataOut_addr, srq.data_length, Phy_DataOut_addr);

	return 0;
}

void watchdog_cb(struct inode * inode, struct file * filp)
{
    mod_timer(&(macp->watchdog_timer), jiffies+(1*HZ));
}


static struct file_operations ts_fops = {
	ioctl:		es_ioctl,
	open:		es_open,
	release:	es_close,
};


/*
 * The driver boot-time initialization code!
 */
static int __init es_init(void)
{
	//int i;
	int ret = 0;	
	//struct esreq *srq = NULL;	

	// initial state
	
	macp = kmalloc(sizeof(struct es_private), GFP_KERNEL);
	macp->regp = (void *) CPE_SECU_VA_BASE;  //john

#ifdef DEBUG 			
	printk("security : macp->regp=0x%08x\n",(u32)macp->regp);
#endif
	
	
	/* Initialize the structure */
	init_timer(&macp->watchdog_timer);
	macp->watchdog_timer.function = (void *) &watchdog_cb;	
		
	/* register our character device */
	if ((ret = register_chrdev(11, "es", &ts_fops)) < 0) {
		printk("can't get major number");
		return ret;
	}
	printk("Security Register OK\n");
		
	DMA_INT_OK = 0;
	rand = jiffies;
	//printk("jiffies = %x\n",jiffies);
	
  	ES_WriteReg(SEC_FIFOThreshold, (1 << 8) + 1);   	

	macp->irq = IRQ_SECU;	 //john
	cpe_int_set_irq(macp->irq, LEVEL, H_ACTIVE);
	if ((ret = request_irq(macp->irq, (void *)es_interrupt, SA_INTERRUPT, "security", macp)) != 0) 
		printk("failed to request_irq\n");	

	ES_WriteReg(SEC_IntrEnable, Data_done);
	
#if 0	
	srq->data_length = 24;	
#endif

#if 0//DES ECB				
	srq->IV_addr[0] = 0x00000000; 	
	srq->IV_addr[1] = 0x00000000; 	
	srq->IV_addr[2] = 0; 		
	srq->IV_addr[3] = 0; 
					
	srq->key_addr[0] = 0x01010101; 	
	srq->key_addr[1] = 0x01010101; 		
	srq->key_addr[2] = 0; 		
	srq->key_addr[3] = 0; 
	
	#if 1
		//DES ECB mode encrypt
		// *DataIn_addr = 0x80000000, 0x00000000, 0x40000000, 0x00000000, 0x20000000, 0x00000000
		// *DataOut_addr = 0x95F8A5E5, 0xDD31D900, 0xDD7F121C, 0xA5015619, 0x2E865310, 0x4F3834EA
		
		*(u32 *)VM_DataIn_addr			=0x00000080;
		*(u32 *)(VM_DataIn_addr + 4)	=0x00000000;
		*(u32 *)(VM_DataIn_addr + 8)	=0x00000040;
		*(u32 *)(VM_DataIn_addr + 12)	=0x00000000;
		*(u32 *)(VM_DataIn_addr + 16)	=0x00000020;
		*(u32 *)(VM_DataIn_addr + 20)	=0x00000000;
	
		//getkey(Algorithm_DES, srq);			
		ES_DMA(Encrypt_Stage, Algorithm_DES, ECB_mode, srq, Tmp_DataIn_addr, Tmp_DataOut_addr, Tmp_data_length);
			
	#else
		//DES ECB mode decrypt
		// *DataIn_addr = 0x95F8A5E5, 0xDD31D900, 0xDD7F121C, 0xA5015619, 0x2E865310, 0x4F3834EA
		// *DataOut_addr = 0x80000000, 0x00000000, 0x40000000, 0x00000000, 0x20000000, 0x00000000
		
		*(u32 *)VM_DataIn_addr			=0xE5A5F895;
		*(u32 *)(VM_DataIn_addr + 4)	=0x00D931DD;
		*(u32 *)(VM_DataIn_addr + 8)	=0x1C127FDD;
		*(u32 *)(VM_DataIn_addr + 12)	=0x195601A5;
		*(u32 *)(VM_DataIn_addr + 16)	=0x1053862E;
		*(u32 *)(VM_DataIn_addr + 20)	=0xEA34384F;
		
		//getkey(Algorithm_DES, srq);			
		ES_DMA(Decrypt_Stage, Algorithm_DES, ECB_mode, srq, Tmp_DataIn_addr, Tmp_DataOut_addr, Tmp_data_length);
				
	#endif
	
#endif
	
#if 0//DES CBC			
	srq->IV_addr[0] = 0x12345678; 	
	srq->IV_addr[1] = 0x90abcdef; 	
	srq->IV_addr[2] = 0; 		
	srq->IV_addr[3] = 0; 
					
	srq->key_addr[0] = 0x01234567; 	
	srq->key_addr[1] = 0x89abcdef; 		
	srq->key_addr[2] = 0; 		
	srq->key_addr[3] = 0; 	
	
	#if 1
		//DES CBC mode encrypt
		// *DataIn_addr = 0x4e6f7720, 0x69732074, 0x68652074, 0x696d6520, 0x666f7220, 0x616c6c20
		// *DataOut_addr = 0xe5c7cdde, 0x872bf27c, 0x43e93400, 0x8c389c0f, 0x68378849, 0x9a7c05f6
		
		*(u32 *)VM_DataIn_addr			=0x20776f4e;
		*(u32 *)(VM_DataIn_addr + 4)	=0x74207369;
		*(u32 *)(VM_DataIn_addr + 8)	=0x0034e943;
		*(u32 *)(VM_DataIn_addr + 12)	=0x0f9c388c;
		*(u32 *)(VM_DataIn_addr + 16)	=0x49883768;
		*(u32 *)(VM_DataIn_addr + 20)	=0xf6057c9a;
	
		//getkey(Algorithm_DES, srq);			
		ES_DMA(Encrypt_Stage, Algorithm_DES, CBC_mode, srq, Tmp_DataIn_addr, Tmp_DataOut_addr, Tmp_data_length);
	
	#else
		//DES CBC mode decrypt
		// *DataIn_addr = 0xe5c7cdde, 0x872bf27c, 0x43e93400, 0x8c389c0f, 0x68378849, 0x9a7c05f6
		// *DataOut_addr = 0x4e6f7720, 0x69732074, 0x68652074, 0x696d6520, 0x666f7220, 0x616c6c20
		
		*(u32 *)VM_DataIn_addr			=0xdecdc7e5;
		*(u32 *)(VM_DataIn_addr + 4)	=0x7cf22b87;
		*(u32 *)(VM_DataIn_addr + 8)	=0x0034e943;
		*(u32 *)(VM_DataIn_addr + 12)	=0x0f9c388c;
		*(u32 *)(VM_DataIn_addr + 16)	=0x49883768;
		*(u32 *)(VM_DataIn_addr + 20)	=0xf6057c9a;
		
		//getkey(Algorithm_DES, srq);			
		ES_DMA(Decrypt_Stage, Algorithm_DES, CBC_mode, srq, Tmp_DataIn_addr, Tmp_DataOut_addr, Tmp_data_length);
		
	#endif
#endif
	
#if 0//DES CTR	

	srq->IV_addr[0] = 0x12345678; 		
	srq->IV_addr[1] = 0x90abcdef; 		
	srq->IV_addr[2] = 0; 		
	srq->IV_addr[3] = 0; 
				
	srq->key_addr[0] = 0x01234567; 		
	srq->key_addr[1] = 0x89abcdef; 			
	srq->key_addr[2] = 0; 		
	srq->key_addr[3] = 0; 	
	srq->key_addr[4] = 0; 
	srq->key_addr[5] = 0; 
	srq->key_addr[6] = 0; 
	srq->key_addr[7] = 0; 
	
	//DES CTR mode
	// *DataIn_addr = 0x4e6f7720, 0x69732074, 0x43e93400, 0x8c389c0f, 0x68378849, 0x9a7c05f6
	// *DataOut_addr = 0xf3096249, 0xc7f46e51, 0x3db698d4, 0x1a9cb508, 0xf4777a9d, 0x7ba806a3
	
	*(u32 *)VM_DataIn_addr			=0x20776f4e;
	*(u32 *)(VM_DataIn_addr + 4)	=0x74207369;
	*(u32 *)(VM_DataIn_addr + 8)	=0x0034e943;
	*(u32 *)(VM_DataIn_addr + 12)	=0x0f9c388c;
	*(u32 *)(VM_DataIn_addr + 16)	=0x49883768;
	*(u32 *)(VM_DataIn_addr + 20)	=0xf6057c9a;
	
	//getkey(Algorithm_DES, srq);			
	ES_DMA(Encrypt_Stage, Algorithm_DES, CTR_mode, srq, Tmp_DataIn_addr, Tmp_DataOut_addr, Tmp_data_length);
	
#endif

//memcpy(&srq->key_addr[0], &key_tmp[0], 32);
//memcpy(&srq->IV_addr[0], &IV_tmp[0], 16);	
#if 0
	printk("key_addr = 0x%08x, data = 0x%08x\n", &srq->key_addr[0], srq->key_addr[0]);
	printk("IV_addr = 0x%08x, data = 0x%08x\n", &srq->IV_addr[0], srq->IV_addr[0]); 			
			
	printk("data_length = 0x%08x, DataIn = 0x%08x, DataOut = 0x%08x\n", (u32)srq->data_length, srq->DataIn_addr, srq->DataOut_addr); 
	printk("VMDataIn = 0x%08x, VMDataOut = 0x%08x\n", VM_DataIn_addr, VM_DataOut_addr); 					
						
	printk("In  = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataIn_addr, *(u32 *)(VM_DataIn_addr+4), *(u32 *)(VM_DataIn_addr+8), *(u32 *)(VM_DataIn_addr+12), *(u32 *)(VM_DataIn_addr+16), *(u32 *)(VM_DataIn_addr+20)); 	
	printk("Out = 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x 0x%08x\n", *(u32 *)VM_DataOut_addr, *(u32 *)(VM_DataOut_addr+4), *(u32 *)(VM_DataOut_addr+8), *(u32 *)(VM_DataOut_addr+12), *(u32 *)(VM_DataOut_addr+16), *(u32 *)(VM_DataOut_addr+20)); 	
	
	printk("reg =");				
	for(i = 0; i < 28; i++)
	{
		if((i % 4) == 0)
			printk("\n");
		printk("%02x 0x%08x ",i*4,readl(macp->regp + i*4));	
	}
	printk("\n");	
	printk("exit es_init\n");	
		
#endif
		
	return 0;
}
 

static u32 random(void)
{
   rand = rand * 0x1664525 + 0x13904223;
   //printk("rand = %x\n",rand);
   return rand;
}
 
static int getkey(int algorithm, struct esreq *srq)
{
	int i, key_length, IV_length;
	u32 IV_addr[4], key_addr[8];

	//printk("getkey\n");
	
	switch (algorithm){
		
		case Algorithm_DES:

			key_length = 8;
			IV_length = 8;
				
			break;
			
		case Algorithm_Triple_DES:

			key_length = 24;
			IV_length = 8;
								
			break;
			
		case Algorithm_AES_128:

			key_length = 16;
			IV_length = 16;
					
			break;
			
		case Algorithm_AES_192:
		
			key_length = 24;
			IV_length = 16;	
				
			break;
			
		case Algorithm_AES_256:
		
			key_length = 32;
			IV_length = 16;
								
			break;								

		default:
			return -ENOIOCTLCMD;
	}		
	
#ifdef DEBUG 			

	IV_addr[0] = 0x00000000; 	
	IV_addr[1] = 0x00000001; 
	IV_addr[2] = 0x00000002; 
	IV_addr[3] = 0x00000003; 
	key_addr[0] = 0x01010101; 		
	key_addr[1] = 0x01010101; 		
	key_addr[2] = 0x01010155;
	key_addr[3] = 0x01010166;				
	key_addr[4] = 0x01010177; 		
	key_addr[5] = 0x01010188; 		
	key_addr[6] = 0x01010199;
	key_addr[7] = 0x010101AA;		
#else
	for(i = 0; i < key_length / 4; i++)
		key_addr[i] = random();

	for(i = 0; i < IV_length / 4; i++)
		IV_addr[i] = random(); 					
#endif

	if (copy_to_user((u32 *)srq->DataIn_addr, key_addr, 32))
	{
		printk("key error to copy_to_user\n");
		return -1;
	}
	
	if (copy_to_user((u32 *)srq->DataOut_addr, IV_addr, 16))
	{
		printk("IV error to copy_to_user\n");
		return -1;
	}	
										
	return 0;
} 

void IV_Output(u32 addr)
{
	int i;
	
	// Output IV value
	for(i = 0; i < 4; i++)
	{
		*(u32 *)(addr + 4 * i) = ES_ReadReg(SEC_LAST_IV0 + 4 * i);	
#ifdef DEBUG 			
		printk("IV = 0x%08x\n",*(u32 *)(addr + 4 * i));	
#endif	
	}
}	
	
static int ES_DMA(int stage, int algorithm, int mode, struct esreq *srq, u32 Tmp_DataIn_addr, u32 Tmp_DataOut_addr, long Tmp_data_length)
{
	int i, section;

	//printk("ES_DMA\n");
	
	Tmp_data_length -= 16;//sub IV return length

	if(mode != CFB_mode)
	{
		switch (algorithm){
			
			case Algorithm_DES:
				if(Tmp_data_length < 8)
				{
					printk("data too short\n");
					return 0;	
				}			
				if((Tmp_data_length % 8) != 0)
					printk("data length not 8 times\n");
				Tmp_data_length -= (Tmp_data_length % 8);
				
				break;
							
			case Algorithm_Triple_DES:
				if(Tmp_data_length < 8)
				{
					printk("data too short\n");
					return 0;	
				}			
				if((Tmp_data_length % 8) != 0)
					printk("data length not 8 times\n");
				Tmp_data_length -= (Tmp_data_length % 8);
				
				break;
				
			case Algorithm_AES_128:
				if(Tmp_data_length < 16)
				{
					printk("data too short\n");
					return 0;	
				}	
				if((Tmp_data_length % 16) != 0)
					printk("data length not 16 times\n");
				Tmp_data_length -= (Tmp_data_length % 16);
	
				break;
				
			case Algorithm_AES_192:
				if(Tmp_data_length < 16)
				{
					printk("data too short\n");
					return 0;	
				}			
				if((Tmp_data_length % 16) != 0)
					printk("data length not 16 times\n");
				Tmp_data_length -= (Tmp_data_length % 24);

				break;
				
			case Algorithm_AES_256:
				if(Tmp_data_length < 16)
				{
					printk("data too short\n");
					return 0;	
				}			
				if((Tmp_data_length % 16) != 0)
					printk("data length not 16 times\n");
				Tmp_data_length -= (Tmp_data_length % 32);
			
				break;								
	
			default:
				return -ENOIOCTLCMD;
		}
	}
		
	if(algorithm == Algorithm_AES_256)
		section = 4064;//16;//value must equal 16 x N
	else if(algorithm == Algorithm_AES_192)
		section = 4064;//16;//value must equal 16 x N
	else if(algorithm == Algorithm_AES_128)
		section = 4064;//16;//value must equal 16 x N
	else			
		section = 4064;//8;//value must equal 8 x N
							
	//1.	Set EncryptControl register
	if(mode == ECB_mode)
		ES_WriteReg(stage, algorithm | mode | stage);
	else
		ES_WriteReg(stage, First_block | algorithm | mode | stage);		
			
	//2.	Set Initial vector IV
	ES_WriteReg(SEC_DESIVH, *(u32 *)srq->IV_addr);
	ES_WriteReg(SEC_DESIVL, *(u32 *)(srq->IV_addr + 1));	
	ES_WriteReg(SEC_AESIV2, *(u32 *)(srq->IV_addr + 2));
	ES_WriteReg(SEC_AESIV3, *(u32 *)(srq->IV_addr + 3));
			
	//3.	Set Key value
	for(i = 0; i < 8; i++)
		ES_WriteReg(SEC_DESKey1H + 4 * i, *(u32 *)(srq->key_addr + i));						
						
	//5.	Set DMA related register
	ES_WriteReg(SEC_DMASrc, Tmp_DataIn_addr);
	ES_WriteReg(SEC_DMADes, Tmp_DataOut_addr);

	
	while(1)
	{	
		
#ifdef DEBUG 			
		printk("Tmp_data_length = %d\n",(u32)Tmp_data_length);	
#endif		
		
		if(Tmp_data_length >= section)
			ES_WriteReg(SEC_DMATrasSize, section);
		else
			ES_WriteReg(SEC_DMATrasSize, Tmp_data_length);
			
		Tmp_data_length -= section;		
	
			
		//6.	Set DmaEn bit of DMAStatus to 1 to active DMA engine
		ES_WriteReg(SEC_DMACtrl, DMA_Enable);	
			
		//7.	Wait transfer size is complete
		while(1)
		{		
			//udelay(100);
			current->state = TASK_INTERRUPTIBLE;
			schedule_timeout(HZ);		
			
			if(DMA_INT_OK == 1)	
			{	
				DMA_INT_OK = 0;
				break;
			}
		}		

		if(Tmp_data_length <= 0)
			break;
			
		// Update IV value
		ES_WriteReg(SEC_DESIVH, ES_ReadReg(SEC_LAST_IV0));
		ES_WriteReg(SEC_DESIVL, ES_ReadReg(SEC_LAST_IV1));	
		ES_WriteReg(SEC_AESIV2, ES_ReadReg(SEC_LAST_IV2));
		ES_WriteReg(SEC_AESIV3, ES_ReadReg(SEC_LAST_IV3));		
	}				
		
	return 0;
} 	

static void __exit es_fini(void) 
{
	printk("es_fini\n");
	unregister_chrdev(11, "es");
	free_irq(macp->irq, macp);
	kfree(macp);
	
}

module_init(es_init);
module_exit(es_fini);

#ifdef MODULE
	MODULE_DESCRIPTION("Security driver");
	MODULE_AUTHOR("Faraday");
	MODULE_LICENSE("Faraday");
#endif


 


