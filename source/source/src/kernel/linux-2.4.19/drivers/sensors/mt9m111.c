//#include <stdint.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-proc.h>
#include <linux/init.h>
#include <video/plmedia.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include "mt9m111.h"

#define		LM_DATE		"20060616"
#define		LM_VERSION	"1.0.0.1"
MODULE_LICENSE("GPL");

// Addresses to scan 
static unsigned short normal_i2c[] = { 0x48, 0x5D, SENSORS_I2C_END };
static unsigned short normal_i2c_range[] = { SENSORS_I2C_END };
static unsigned int normal_isa[] = { SENSORS_ISA_END };
static unsigned int normal_isa_range[] = { SENSORS_ISA_END };

// Insmod parameters 
SENSORS_INSMOD_1(MT9M111);


static int MT9M111AttachAdapter(struct i2c_adapter *adapter);
static int MT9M111Detect(struct i2c_adapter *adapter, int address, unsigned short flags, int kind);
static int MT9M111DetachClient(struct i2c_client *client);
static void MT9M111ProcRead(struct i2c_client *client, int operation, int ctl_name, int *nrels_mag, long *results);
static void MT9M111ProcWrite(struct i2c_client *client, int operation, int ctl_name, int *nrels_mag, long *results);
static void MT9M111UpdateClient(struct i2c_client *client);
static void MT9M111InitClient(struct i2c_client *client);
static int MT9M111ReadWord(struct i2c_client *client, int reg);
static int MT9M111WriteWord(struct i2c_client *client, int reg, int val);
 
static int MT9M111SetFormat(PSENSOR pSensor);
static int MT9M111GetFormat(PSENSOR pSensor);
static int MT9M111SetSync(PSENSOR pSensor);
static int MT9M111GetSync(PSENSOR pSensor);
static void MT9M111Restart(PSENSOR pSensor);

static int SensorOpen(struct inode *pinode, struct file *pfile);
static int SeneorRelese(struct inode *pinode, struct file *pfile);
static int SensorIOCTL(struct inode *pinode, struct file *pfile, 
	unsigned int cmd, unsigned long arg);

static const char MT9M111_SHORT_NAME[]="MT9M111";
static const char MT9M111_FULL_NAME[]="Micron MT9M111 SXGA Sensor";
static const char MT9M111_VERSION[]="1.0.0.1";

static struct file_operations SensorFOPS=
{
	owner:		THIS_MODULE,
	open:		SensorOpen,
	release:	SeneorRelese,
	ioctl:		SensorIOCTL,
};
	
//===========================================================================
//	CONTEXT A
REGISTERINFO MT9M111CameraControlA[] =
{
	{SHDREG_PG_MAP, 			0x0002},	//	switch to page 2;
	{PG2REG_AE_SPD_SEN_CTL_A,	0xDF20}		//	[8..6]:100, [2..0]:000
};
			
REGISTERINFO MT9M111ColorpipeA[] =
{
	{SHDREG_PG_MAP, 			0x0001},	//	switch to page 1;
	{PG1REG_O_FMT_CTL2_A, 		0x0200},	//	[9]:1
    {PG1REG_DEFECT_CORR_A, 		0x0000},
    {PG1REG_REDU_H_PAN_A, 		0x0000},
    {PG1REG_REDU_H_ZOOM_A, 		0x0500},
    {PG1REG_REDU_H_O_SIZE_A, 	0x0280},
    {PG1REG_REDU_V_PAN_A, 		0x0000},
    {PG1REG_REDU_V_ZOOM_A, 		0x0400},
    {PG1REG_REDU_V_O_SIZE_A, 	0x0200}
};

REGISTERINFO MT9M111SensorCoreA[] =
{
	{SHDREG_PG_MAP, 			0x0000},		//switch to page 0;
    {PG0REG_H_BLANK_A, 			0x00BE},
    {PG0REG_V_BLANK_A, 			0x0011},
    {PG0REG_RD_MODE_A, 			0x040C}
};
//
//===========================================================================

//===========================================================================
//	CONTEXT B
REGISTERINFO MT9M111CameraControlB[] =
{
	{SHDREG_PG_MAP, 0x0002},		//	switch to page 2;
	{PG2REG_AE_SPD_SEN_CTL_B,	0xDF20}		//	[8..6]:100, [2..0]:000
};
			
REGISTERINFO MT9M111ColorpipeB[] =
{
	{SHDREG_PG_MAP, 0x0001},		//	switch to page 1;
	{PG1REG_O_FMT_CTL2_B, 		0x0200},	//	[11]:1; [9]:1
    {PG1REG_DEFECT_CORR_B, 		0x0000},
    {PG1REG_REDU_H_PAN_B, 		0x0000},
    {PG1REG_REDU_H_ZOOM_B, 		0x0500},
    {PG1REG_REDU_H_O_SIZE_B, 	0x0500},
    {PG1REG_REDU_V_PAN_B, 		0x0000},
    {PG1REG_REDU_V_ZOOM_B, 		0x0400},
    {PG1REG_REDU_V_O_SIZE_B, 	0x0400}
};

REGISTERINFO MT9M111SensorCoreB[] =
{
	{SHDREG_PG_MAP, 			0x0000},		//switch to page 0;
    { PG0REG_H_BLANK_B,			0x0184},
    { PG0REG_V_BLANK_B,			0x002A},
    { PG0REG_RD_MODE_B,			0xC300}
};
//
//===========================================================================

//===========================================================================
// Common
REGISTERINFO MT9M111CameraControlC[] =
{
	{SHDREG_PG_MAP, 			0x0002},		//switch to page 2;
    {PG2REG_AE_WIN_H_BOUND, 	0x8000},
    {PG2REG_AE_WIN_V_BOUND, 	0x8008},
    {PG2REG_AE_CWIN_H_BOUND, 	0x6020},
    {PG2REG_AE_CWIN_V_BOUND, 	0x6020},
    {PG2REG_AWB_WIN_BOUND, 		0xF0A0},
	{0x37,						0x0080},	//	fixed 30fps
    {PG2REG_AE_TG_PREC_CTL, 	0x0C4A},
	{PG2REG_FLICKER_CTL, 		0x0002},	//	[0]:0--> Auto flicker detection
//    {PG2REG_AE_DIG_GAIN_MON, 	0x0000},	//	be used while AE is disabled
	{PG2REG_AE_DIG_GAIN_LIM, 	0x4010},	//	post->4.0, per->1.0
};
			
REGISTERINFO MT9M111ColorpipeC[] =
{
	{SHDREG_PG_MAP, 			0x0001},		//switch to page 1;
	{PG1REG_APERTURE_CORR, 		0x0003},	
	{PG1REG_OP_MODE_CTL, 		0x748E},
	{PG1REG_SATURATION_CTL,		0x0005},
	{PG1REG_LUMA_OFFSET,		0x0010},
	{PG1REG_LUMA_CLIP,			0xF010},
	{PG1REG_TEST_CTL,			0x0000},
	{PG1REG_REDU_ZOOM_SS,		0x0504},
	{PG1REG_REDU_ZOOM_CTL,		0x0010},
	{PG1REG_GBL_CLK_CTL,		0x0002},
	{PG1REG_EFFECTS_MODE,		0x7000},	//	[2:0]:000-->no effect
	{PG1REG_EFFECTS_SEPIA,		0xB023}
};

REGISTERINFO MT9M111SensorCoreC[] =
{
	{SHDREG_PG_MAP, 			0x0000},	//switch to page 0;
	{PG0REG_ROW_START,			0x000C},
	{PG0REG_COL_START,			0x001E},
	{PG0REG_WIN_HEIGHT, 		0x0400},
	{PG0REG_WIN_WIDTH,	 		0x0500},
	{PG0REG_SHUTTER_WIDTH,		0x0219},
	{PG0REG_ROW_SPEED,			0x0011},
	{PG0REG_EX_DELAY,			0x0000},
	{PG0REG_SHUTTER_DELAY,		0x0000},
//    {PG0REG_FLASH_CTL,		},			//	[15]:?--> dectect flash PIN to active this register
	{PG0REG_GREEN1_GAIN,		0x0020},
	{PG0REG_BLUE_GAIN,			0x0020},
	{PG0REG_RED_GAIN,			0x0020},
	{PG0REG_GREEN2_GAIN,		0x0020},
	{PG0REG_GLOBAL_GAIN,		0x0020}
};
//
//===========================================================================

//===========================================================================
//
REGISTERINFO *DefaultSetting[3][3]=
{	//	context A
	{
		MT9M111SensorCoreA,
		MT9M111ColorpipeA,
		MT9M111CameraControlA
	},
	//	context B
	{
		MT9M111SensorCoreB,
		MT9M111ColorpipeB,
		MT9M111CameraControlB
	},
	//	common
	{
		MT9M111SensorCoreC,
		MT9M111ColorpipeC,
		MT9M111CameraControlC
	}
};
//
//===========================================================================

static ctl_table MT9M111_dir_table_template[] = 
{
	{
		MT9M111_SYSCTL_READ, 
		"read", 
		NULL, 
		0, 
		0444, 
		NULL, 
		&i2c_proc_real,
		&i2c_sysctl_real, 
		NULL, 
		&MT9M111ProcRead
	},
	{
		MT9M111_SYSCTL_WRITE, 
		"write", 
		NULL, 
		0, 
		0644, 
		NULL, 
		&i2c_proc_real,
		&i2c_sysctl_real, 
		NULL, 
		&MT9M111ProcWrite
	},
	{0}
};

PMT9M111DATA g_pMT9M111Data;
PSENSOR		g_pPLSensor;
static int 	MT9M111_id = 0;

static struct i2c_driver MT9M111Driver=
{
	.owner		= THIS_MODULE,
	.name		= "MT9M111 sensor chip driver",	//	max. 40 characters limited
	.id			= I2C_DRIVERID_MT9M111,			//	usually from i2c-id.h
	.flags		= I2C_DF_NOTIFY,				//	always set this value
 	.attach_adapter	= MT9M111AttachAdapter,		
	.detach_client	= MT9M111DetachClient,
};


static int MT9M111ReadWord(struct i2c_client *client, int reg)
{
	unsigned short data;
	data = i2c_smbus_read_word_data(client, reg);
	return (((data & 0xff)<< 8) | (data >> 8));
}

static int MT9M111WriteWord(struct i2c_client *client, int reg, int val)
{
	unsigned short data;
	data = ((val & 0xff) << 8) | (val >> 8);
//	data = val;
//	printk("<1>MT9M111WriteWord(): client = %p\n", client);
	return i2c_smbus_write_word_data(client, reg, data);
}

void MT9M111ProcRead(struct i2c_client *client, int operation,
	int ctl_name, int *nrels_mag, long *results)
{
	PMT9M111DATA data = client->data;
	
//	printk("<1>MT9M111ProcRead():i2c_client = %p\n", client);
	if (operation == SENSORS_PROC_REAL_INFO)
		*nrels_mag = 0;
	else if (operation == SENSORS_PROC_REAL_READ) 
	{
		MT9M111UpdateClient(client);
		results[0] = data->wRead;
		*nrels_mag = 1;
	}  
}

void MT9M111ProcWrite(struct i2c_client *client, int operation,
		    int ctl_name, int *nrels_mag, long *results)
{
	PMT9M111DATA	data = client->data;
	
//	printk("<1>MT9M111ProcWrite() be called\n");
	if (operation == SENSORS_PROC_REAL_INFO)
		*nrels_mag = 0;
	else if (operation == SENSORS_PROC_REAL_READ) 
	{
		results[0] = data->wReg;
		results[1] = data->wWrite; 
		*nrels_mag = 2;
	} 
	else if (operation == SENSORS_PROC_REAL_WRITE) 
	{
		if (*nrels_mag == 2) 
		{
			data->wReg = results[0];
			data->wWrite = results[1];
			MT9M111WriteWord(client, data->wReg, data->wWrite);
		}
		
	}
}

static void HWResetSensor(struct i2c_client *client)
{
	MT9M111WriteWord(client, SHDREG_PG_MAP, 0x0000);	// change to page 0
	MT9M111WriteWord(client, PG0REG_RESET, 	0x0001);	//	set reset bit [0]<--1
	MT9M111WriteWord(client, PG0REG_RESET,	0x0008);	//	clear reset bit [0]<--0
}

static int MT9M111AttachAdapter(struct i2c_adapter *adapter)
{
	int rc;
//	printk("<1>MT9M111AttachAdapter() be called\n");
	rc=i2c_detect(adapter, &addr_data, MT9M111Detect);
	if(rc<0)
		printk("<1>call i2c_detect() failed\n");
	return rc;
}

//	This function is called by i2c_detect 
int MT9M111Detect(struct i2c_adapter *adapter, int address, unsigned short flags, int kind)
{
	struct i2c_client	*new_client;
	PMT9M111DATA 		data;
	int 				err = 0, i, rc;
	const char			*type_name, *client_name;

//	printk("<1>MT9M111Detect() be called\n");
	//	Make sure we aren't probing the ISA bus!! This is just a safety check
	//  at this moment; i2c_detect really won't call us.
#ifdef DEBUG
	if (i2c_is_isa_adapter(adapter)) 
	{
		printk("MT9M111.o: MT9M111Detect called for an ISA bus adapter?!?\n");
		return 0;
	}
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE | 
		I2C_FUNC_SMBUS_BYTE_DATA))
		goto ERROR0;

	//	OK. For now, we presume we have a valid client. We now create the
	//  client structure, even though we cannot fill it completely yet.
	//  But it allows us to access MT9M111_{read,write}_value.
	if (!(new_client = kmalloc(sizeof(struct i2c_client)+
		 sizeof(MT9M111DATA)+sizeof(SENSOR), GFP_KERNEL))) 
	{
		err = -ENOMEM;
		goto ERROR0;
	}

	g_pMT9M111Data= data = (MT9M111DATA *) (new_client + 1);
	g_pPLSensor = data->pSensor =(PSENSOR)(data+1);
	new_client->addr = address;
	new_client->data = data;
	new_client->adapter = adapter;
	new_client->driver = &MT9M111Driver;
	new_client->flags = 0;

	//	Now, we would do the remaining detection. But the MT9M111 is plainly
	//  impossible to detect! Stupid chip.
	// Determine the chip type
	if (kind <= 0) 
	{
		if (address == 0x48 || address == 0x5D)
		{
			MT9M111WriteWord(new_client, SHDREG_PG_MAP, 0);
			rc=MT9M111ReadWord(new_client, PG0REG_CHIP_VER_00);
			if(rc != MT9M111_CHIP_VER)
			{
				printk("<1>Unsupported sensor; chip version = 0x%04X\n", rc);
				goto ERROR1;
			}
			kind = MT9M111;
			type_name = "MT9M111";
			client_name = "MT9M111 chip";
		}
		else 
			goto ERROR1;
	}
	else if (kind == MT9M111) 
	{
		type_name = "MT9M111";
		client_name = "MT9M111 chip";
	} 
	else 
	{
#ifdef DEBUG
		printk("MT9M111.o: Internal error: unknown kind (%d)?!?", kind);
#endif
		goto ERROR1;
	}

	// Fill in the remaining client fields and put it into the global list 
	strcpy(new_client->name, client_name);

	new_client->id = MT9M111_id++;
	data->cValid = 0;
	init_MUTEX(&data->UpdateLock);

	// Tell the I2C layer a new client has arrived
	if ((err = i2c_attach_client(new_client)))
		goto ERROR3;

	// Register a new directory entry with module sensors
	if ((i = i2c_register_entry(new_client, type_name, 
		MT9M111_dir_table_template)) < 0) 
	{
		err = i;
		goto ERROR4;
	}
	data->nSysCtlID = i;

	//Initialize the MT9M111D chip
    MT9M111InitClient(new_client);
	return 0;

//	OK, this is not exactly good programming practice, usually. But it is
//	very code-efficient in this case.

ERROR4:
	i2c_detach_client(new_client);
ERROR3:
ERROR1:
	kfree(new_client);
ERROR0:
	return err;
}


static int MT9M111DetachClient(struct i2c_client *client)
{
	int err;

//	printk("<1>MT9M111DetachClient() be called\n");
	i2c_deregister_entry(((PMT9M111DATA) (client->data))-> nSysCtlID);

	if ((err = i2c_detach_client(client))) 
	{
		printk("MT9M111.o: Client deregistration failed, client not detached.\n");
		return err;
	}
	kfree(client);
	return 0;

}
/* Called when we have found a new MT9M111. */

static void MT9M111InitClient(struct i2c_client *client)
{
	PMT9M111DATA 	data = client->data;
	int				i, j, k;

	data->pSensor->nType		= TYPE_SENSOR;	
	data->pSensor->pI2CClient	= client;
    data->pSensor->szShortName	= MT9M111_SHORT_NAME;
    data->pSensor->szFullName	= MT9M111_FULL_NAME;
    data->pSensor->szVersion	= MT9M111_VERSION;
	data->pSensor->PrivateData	= data;
	data->pSensor->Restart		= MT9M111Restart;
	data->pSensor->SetFormat	= MT9M111SetFormat;
	data->pSensor->GetFormat	= MT9M111GetFormat;
	data->pSensor->SetSync		= MT9M111SetSync;
	data->pSensor->GetSync		= MT9M111GetSync;
	
	data->pSensor->SyncInfo.nHSyncStart		= 0;
	data->pSensor->SyncInfo.nVSyncStart		= 0;
	data->pSensor->SyncInfo.byITUBT656		= 0;    /* embedded sync */
	data->pSensor->SyncInfo.byHRefSelect	= 1;
	data->pSensor->SyncInfo.nHSyncPhase		= 0;
	data->pSensor->SyncInfo.byHSyncState	= 0;
	data->pSensor->SyncInfo.byVSyncState	= 0;
	data->pSensor->SyncInfo.byValidEnable	= 0;
	data->pSensor->SyncInfo.byValidState	= 0;

	data->DevFS = devfs_register (NULL, "sensor", 
		DEVFS_FL_AUTO_DEVNUM, 0, 0,	//	auto major and minor number
		S_IFCHR | S_IWUSR | S_IRUSR, &SensorFOPS, NULL);
	HWResetSensor(client);
	for(i=0; i<3; i++)
		for(j=0; j<3; j++)
			for (k = 0; k< sizeof(DefaultSetting[i][j])/sizeof(DefaultSetting[i][j][0]); k++)
				MT9M111WriteWord(client, 
					DefaultSetting[i][j][k].uRegister, 
					DefaultSetting[i][j][k].uValue);
	printk("<1>CMOS sensor %s is avaliable now\n", data->pSensor->szShortName);
//	data->byWrite = MT9M111_INIT;
//	i2c_smbus_write_byte(client, data->byWrite);
}


static void MT9M111UpdateClient(struct i2c_client *client)
{
	MT9M111DATA *data = client->data;

	down(&data->UpdateLock);

	if ((jiffies - data->uLastUpdated > 5*HZ) ||
	    (jiffies < data->uLastUpdated) || !data->cValid) 
	{
#ifdef DEBUG
		printk("Starting MT9M111 update\n");
#endif
		data->wRead = i2c_smbus_read_byte(client); 
		data->uLastUpdated = jiffies;
		data->cValid = 1;
	}
	up(&data->UpdateLock);
}

static int __init sm_MT9M111_init(void)
{
	printk("<1>MT9M111.o version %s (%s)\n", LM_VERSION, LM_DATE);
	return i2c_add_driver(&MT9M111Driver);
}

static void __exit sm_MT9M111_exit(void)
{
	i2c_del_driver(&MT9M111Driver);
}

//===========================================================================
//
// Each client has this additional data

static int SensorOpen(struct inode *pinode, struct file *pfile)
{
	return 0;
}

static int SeneorRelese(struct inode *pinode, struct file *pfile)
{
	return 0;
}

static int SensorIOCTL(struct inode *pinode, struct file *pfile, 
	unsigned int cmd, unsigned long arg)
{

	struct i2c_client	*pClient=g_pPLSensor->pI2CClient;
	REGISTERINFO		reg;
	
	switch (cmd)
	{
		case PLSENSOR_WRITE_REGISTER:
			copy_from_user((void*)&reg , (void*)arg, sizeof(REGISTERINFO));
			MT9M111WriteWord(pClient, reg.uRegister, reg.uValue);
			break;
		case PLSENSOR_READ_REGISTER:
			copy_from_user((void*)&reg , (void*)arg, sizeof(REGISTERINFO));
			reg.uValue=MT9M111ReadWord(pClient, reg.uRegister);
			copy_to_user((void*)arg, (void*)&reg, sizeof(REGISTERINFO));
			break;
		case PLSENSOR_WRRD_REGISTER:
			copy_from_user((void*)&reg , (void*)arg, sizeof(REGISTERINFO));
			MT9M111WriteWord(pClient, reg.uRegister, reg.uValue);
			reg.uValue=MT9M111ReadWord(pClient, reg.uRegister);
			copy_to_user((void*)arg, (void*)&reg, sizeof(REGISTERINFO));
			break;
	}
	return 0;
}

static int MT9M111SetFormat(PSENSOR pSensor)
{
	struct i2c_client	*pClient=pSensor->pI2CClient;
    int 				rc=0;
	
	if (pSensor->Format.nCropWidth <= 640 && pSensor->Format.nCropHeight <= 512)
    {	// low power viewfind mode
		
		MT9M111WriteWord(pClient, SHDREG_PG_MAP, 0x0000);	// change to page 0
		udelay(10);
		MT9M111WriteWord(pClient, PG0REG_RESET, 0x8008);	//	set reset bit [0]<--1
		udelay(10);

		rc=MT9M111WriteWord(pClient, SHDREG_PG_MAP, 0x0001);		// change to page 1
		if(rc<0)
			printk("<1>write %d to SHDREG_PG_MAP of MT9M111 via i2c failed\n", 0x0001);
		udelay(10);
    	
		rc=MT9M111WriteWord(pClient, PG1REG_REDU_H_PAN_A, 0x0000);
		if(rc<0)
			printk("<1>write %d to PG1REG_REDU_H_PAN_A of MT9M111 via i2c failed\n", 0x0000);
		udelay(10);
   	
		rc=MT9M111WriteWord(pClient, PG1REG_REDU_H_O_SIZE_A, 	pSensor->Format.nCropWidth);
		if(rc<0)
			printk("<1>write %d to PG1REG_REDU_H_O_SIZE_A of MT9M111 via i2c failed\n",
				pSensor->Format.nCropWidth);
 		udelay(10);
   	
		rc=MT9M111WriteWord(pClient, PG1REG_REDU_V_PAN_A, 0x0000);
		if(rc<0)
			printk("<1>write %d to PG1REG_REDU_V_PAN_A of MT9M111 via i2c failed\n", 0x0000);
		udelay(10);
	
		rc=MT9M111WriteWord(pClient, PG1REG_REDU_V_O_SIZE_A, 	pSensor->Format.nCropHeight);
		if(rc<0)
			printk("<1>write %d to PG1REG_REDU_V_O_SIZE_A of MT9M111 via i2c failed\n",
				pSensor->Format.nCropHeight);
		udelay(10);
//		TEST pattern		
//		rc=MT9M111WriteWord(pClient, PG1REG_TEST_CTL, 7);
		
		rc=MT9M111WriteWord(pClient, SHDREG_PG_MAP, 0x0002);	// change to page 2
		if(rc<0)
			printk("<1>write %d to SHDREG_PG_MAP of MT9M111 via i2c failed\n", 2);
		udelay(10);
		
		rc=MT9M111WriteWord(pClient, PG2REG_SHUT_WLIM_AE, 	0x0080);
		if(rc<0)
			printk("<1>write %d to PG2REG_SHUT_WLIM_AE of MT9M111 via i2c failed\n", 0x0080);
        udelay(10);
        
		rc=MT9M111WriteWord(pClient, SHDREG_CTXT_CTL, 0x8000);
		if(rc<0)
			printk("<1>write %d to SHDREG_CTXT_CTL of MT9M111 via i2c failed\n", 0x8000);
		udelay(10);
			
		MT9M111WriteWord(pClient, SHDREG_PG_MAP, 	0x0000);	// change to page 0
		udelay(10);
		MT9M111WriteWord(pClient, PG0REG_RESET, 	0x0008);	//	set reset bit [0]<--1
        udelay(10);
		pSensor->Format.nFPS=30;
		pSensor->Format.nInterlaced=0;
		return 0;
    }
    return -1;
}

static int MT9M111GetFormat(PSENSOR pSensor)
{
	return 0;	
}

static int MT9M111SetSync(PSENSOR pSensor)
{
	int		context, value;
	
	MT9M111WriteWord(pSensor->pI2CClient, SHDREG_PG_MAP, 0x0002);	// change to page 2
	value=MT9M111ReadWord(pSensor->pI2CClient, SHDREG_CTXT_CTL);	//	set reset bit [0]<--1
	context=(value & (1<<9))?1:0;	// bit[9]: 1-->context B , 0-->context A
	MT9M111WriteWord(pSensor->pI2CClient, SHDREG_PG_MAP, 0x0001);   //
	if(context)
	{	// Context B
		if(pSensor->SyncInfo.byHRefSelect==DONT_CARE)
    	{
			pSensor->SyncInfo.byITUBT656=0;
			pSensor->SyncInfo.byHRefSelect=1;
			value=MT9M111ReadWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_B);
			value &= 0xF7FF;	//	set external itu656
			MT9M111WriteWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_B, value);
    	}
    	else
    	{
	    	if(!pSensor->SyncInfo.byHRefSelect)
    		{
				pSensor->SyncInfo.byITUBT656=1;
				value=MT9M111ReadWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_B);
				value |= 0x0800;	//	set embedded itu656
				MT9M111WriteWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_B, value);
			}
			else
			{	// using href
				pSensor->SyncInfo.byITUBT656=0;
			value=MT9M111ReadWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_B);
			value &= 0xF7FF;	//	set external itu656
			MT9M111WriteWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_B, value);
		}
	}
	}
	else
	{	// Context A
//		printk("<1>Context A : HRefSelect = 0x%02X\n", pSensor->SyncInfo.byHRefSelect);  
		if(pSensor->SyncInfo.byHRefSelect==DONT_CARE)
    	{
			printk("<1>Don\'t care\n");
			pSensor->SyncInfo.byITUBT656=0;
			pSensor->SyncInfo.byHRefSelect=1;
			value=MT9M111ReadWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_A);
			value &= 0xF7FF;	//	set external itu656
			MT9M111WriteWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_A, value);
		}
		else
		{
//			printk("<1>Hummm~~...BUG\n");
			if(!pSensor->SyncInfo.byHRefSelect)
			{	
				pSensor->SyncInfo.byITUBT656=1;
			value=MT9M111ReadWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_A);
			value |= 0x0800;	//	set embedded itu656
			MT9M111WriteWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_A, value);
    	}
    	else
    	{
				pSensor->SyncInfo.byITUBT656=0;
			value=MT9M111ReadWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_A);
			value &= 0xF7FF;	//	set external itu656
			MT9M111WriteWord(pSensor->pI2CClient, PG1REG_O_FMT_CTL2_A, value);
		}
	}
	}
	
	pSensor->SyncInfo.byHSyncState=0;
	pSensor->SyncInfo.byVSyncState=1;
	return 0;
}

static int MT9M111GetSync(PSENSOR pSensor)
{
	return 0;	
}

static void MT9M111Restart(PSENSOR pSensor)
{
	struct i2c_clien	*pClient=pSensor->pI2CClient;
	int					i, j, k;
	
	HWResetSensor(pClient);
	for(i=0; i<3; i++)
		for(j=0; j<3; j++)
			for (k = 0; k< sizeof(DefaultSetting[i][j])/sizeof(DefaultSetting[i][j][0]); k++)
				MT9M111WriteWord(pClient, 
					DefaultSetting[i][j][k].uRegister, 
					DefaultSetting[i][j][k].uValue);
}

//
//===========================================================================

MODULE_AUTHOR("Chen Chao Yi<joe-chen@prolific.com.tw>");
MODULE_DESCRIPTION("MT9M111 driver");
EXPORT_SYMBOL(g_pPLSensor);
module_init(sm_MT9M111_init);
module_exit(sm_MT9M111_exit);


