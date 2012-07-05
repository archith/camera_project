//#include <stdint.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/i2c-proc.h>
#include <linux/init.h>
#include <linux/devfs_fs_kernel.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include "tw9906.h"

#define		LM_DATE		"20060619"
#define		LM_VERSION	"1.0.0.2a"
MODULE_LICENSE("GPL");

// Addresses to scan 
static unsigned short normal_i2c[] = { TW9906_I2C_IOADDR, SENSORS_I2C_END };
static unsigned short normal_i2c_range[] = { SENSORS_I2C_END };
static unsigned int normal_isa[] = { SENSORS_ISA_END };
static unsigned int normal_isa_range[] = { SENSORS_ISA_END };

// Insmod parameters 
SENSORS_INSMOD_1(TW9906);


static int TW9906AttachAdapter(struct i2c_adapter *adapter);
static int TW9906Detect(struct i2c_adapter *adapter, int address, unsigned short flags, int kind);
static int TW9906DetachClient(struct i2c_client *client);
static void TW9906ProcRead(struct i2c_client *client, int operation, int ctl_name, int *nrels_mag, long *results);
static void TW9906ProcWrite(struct i2c_client *client, int operation, int ctl_name, int *nrels_mag, long *results);
static void TW9906UpdateClient(struct i2c_client *client);
static void TW9906InitClient(struct i2c_client *client);
 
static int TW9906SetFormat(PSENSOR pSensor);
static int TW9906GetFormat(PSENSOR pSensor);
static int TW9906SetSync(PSENSOR pSensor);
static int TW9906GetSync(PSENSOR pSensor);
static void TW9906Restart(PSENSOR pSensor);
//#ifdef __MEDIA_PUMP__
static int TW9906SetInput(PSENSOR pSensor);
static int TW9906GetInput(PSENSOR pSensor);
static int TW9906SetVSTD(PSENSOR pSensor);
static int TW9906GetVSTD(PSENSOR pSensor);
static int TW9906ShowStatus(PSENSOR pSensor);
//#endif
static int SensorOpen(struct inode *pinode, struct file *pfile);
static int SeneorRelese(struct inode *pinode, struct file *pfile);
static int SensorIOCTL(struct inode *pinode, struct file *pfile, 
	unsigned int cmd, unsigned long arg);

static const char TW9906_SHORT_NAME[]="TW9906";
static const char TW9906_FULL_NAME[]="Techwell TW9906 video decoder";
static const char TW9906_VERSION[]="1.0.0.1";

static struct file_operations SensorFOPS=
{
	owner:		THIS_MODULE,
	open:		SensorOpen,
	release:	SeneorRelese,
	ioctl:		SensorIOCTL,
};
	
//===========================================================================
//	Default value of all registers

REGISTERINFO DefaultSetting[] = 
{
    {TW9906REG_INFORM, 		TW9906COMPOSITE_SEL},	// 0x02 Input Format
    {TW9906REG_OPFORM, 		0x80},	// 0x03 Output Format Control Register
//    {TW9906REG_GAMMA_HSYNC, 0x00},	// 0x04 Gamma and HSync Delay Control
    {TW9906REG_OCNTL1, 		0x88},		// 0x05 Output Polarity Register
    {TW9906REG_ACNTL, 		0x00},	// 0x06 Analog Control Register
    {TW9906REG_CROP_HI, 	0x02},  // 0x07 Cropping Register, High
    {TW9906REG_VDELAY_LO, 	0x14},  // 0x08 Vertical Delay Register, Low
    {TW9906REG_VACTIVE_LO, 	0xF0},  // 0x09 Vertical Active Register, Low
    {TW9906REG_HDELAY_LO, 	0x88},  // 0x0A Horizontal Delay Register, Low
    {TW9906REG_HACTIVE_LO, 	0xC0},  // 0x0B Horizontal Active Register, Low
    {TW9906REG_CNTRL1, 		0xCF},  // 0x0C Control Register I
    {TW9906REG_VSCALE_LO, 	0x00},  // 0x0D Vertical Scaling Register, Low
    {TW9906REG_SCALE_HI, 	0x11},  // 0x0E Scaling Register, High
    {TW9906REG_HSCALE_LO, 	0x09},  // 0x0F Horizontal Scaling Register, Low
    {TW9906REG_BRIGHT, 		0x00},  // 0x10 Brightness Control Register
    {TW9906REG_CONTRAST, 	0x60},  // 0x11 Contrast Control Register
    {TW9906REG_SHARPNESS, 	0x50},  // 0x12 SHARPNESS Control Register I
    {TW9906REG_SAT_U, 		0x7F},  // 0x13 Chroma (U) Gain Register
    {TW9906REG_SAT_V, 		0x5A},  // 0x14 Chroma (V) Gain Register
    {TW9906REG_HUE, 		0x00},  // 0x15 Hue Control Register
//	{TW9906REG_SHARPNESS2, 	0xC5},  // 0x16 Sharpness Control Register II
    {TW9906REG_VSHARP, 		0x80},  // 0x17 Vertical Peaking Control I
    {TW9906REG_CORING, 		0x44},  // 0x18 Coring Control Register
    {TW9906REG_VBICNTL, 	0x58},  // 0x19 VBI Control Register
    {TW9906REG_ANALOGCTRL1,	0x00},  // 0x1A CC/EDS Status Register
    {TW9906REG_OCNTL2,		0x80},  // 0x1B CC/EDS Data Register
    {TW9906REG_SDT, 		0x07},  // 0x1C Standard Selection
    {TW9906REG_SDTR, 		0x7F},  // 0x1D Standard Recongnition
    {TW9906REG_CVFMT, 		0x08},  // 0x1E General Programmable I/O Register (GPIO)
    {TW9906REG_TEST, 		0x00},  // 0x1F Test Control Register
    {TW9906REG_CLMPG, 		0x80},  // 0xA9,	// 0x20 Clamping Gain
    {TW9906REG_IAGC, 		0x22},  // 0x21 Individual AGC Gain
    {TW9906REG_AGCGAIN, 	0xF0},  // 0x22 AGC Gain
    {TW9906REG_PEAKWT, 		0xF8},  // 0x23 White Peak Threshold
    {TW9906REG_CLMPL, 		0x3C},  // 0x24 Clamp level
    {TW9906REG_SYNCT, 		0x38},  // 0x25 Sync Amplitude
    {TW9906REG_MISSCNT, 	0x44},  // 0x26 Sync Miss Count Register
    {TW9906REG_PCLAMP, 		0x20},  // 0x27 Clamp Position Register
    {TW9906REG_VCNTL1, 		0x00},  // 0x28 Vertical Control Register
    {TW9906REG_VCNTL2, 		0x15},  // 0x29 Vertical Control II
    {TW9906REG_CKILL,	 	0x68},  // 0x2A Color Killer Level Control
    {TW9906REG_COMB,		0x33},  // 0x2B Comb Filter Control
    {TW9906REG_LDLY, 		0x30},  // 0x2C Luma Delay and HSYNC Control
    {TW9906REG_MISC1, 		0x10},  // 0x2D Miscellaneous Control Register I
    {TW9906REG_LOOP, 		0xA5},  // 0x2E Miscellaneous Control Register II
    {TW9906REG_MISC2, 		0xE0},  // 0x2F Miscellaneous Control III
//    {TW9906REG_MVSN, 		0x00},  // 0x50,	// 0x30 Macrovision Detection
//    {TW9906REG_STATUS2, 	0x00},  // 0x31 Clamp Cntl2 
//    {TW9906REG_HFREF,		0xA0},  // 0x32 Fill Data
    {TW9906REG_CLMD,		0x05},  // 0x33 VBI Cntl1
    {TW9906REG_IDCNTL,		0x1E},  // 0x34 VBI Cntl2
    {TW9906REG_CLCNTL1, 	0x00},  // 0x35 Misc4
    {TW9906REG_SDID, 		0x22},  // 0x35 Misc4
    {TW9906REG_DID, 		0x31},  // 0x35 Misc4
    {TW9906REG_VVBI, 		0x00},  // 0x35 Misc4
    {TW9906REG_OVSEND, 		0x88},  // 0x35 Misc4
};
//
//===========================================================================

//===========================================================================
//
static ctl_table TW9906_dir_table_template[] = 
{
	{
		TW9906_SYSCTL_READ, 
		"read", 
		NULL, 
		0, 
		0444, 
		NULL, 
		&i2c_proc_real,
		&i2c_sysctl_real, 
		NULL, 
		&TW9906ProcRead
	},
	{
		TW9906_SYSCTL_WRITE, 
		"write", 
		NULL, 
		0, 
		0644, 
		NULL, 
		&i2c_proc_real,
		&i2c_sysctl_real, 
		NULL, 
		&TW9906ProcWrite
	},
	{0}
};

PTW9906DATA g_pTW9906Data;
PSENSOR		g_pPLSensor;
static int 	TW9906_id = 0;

static struct i2c_driver TW9906Driver=
{
	.owner			= THIS_MODULE,
	.name			= "TW9906 video decoder driver",	//	max. 40 characters limited
	.id				= I2C_DRIVERID_TW9906,			//	usually from i2c-id.h
	.flags			= I2C_DF_NOTIFY,				//	always set this value
 	.attach_adapter	= TW9906AttachAdapter,		
	.detach_client	= TW9906DetachClient,
};

void TW9906ProcRead(struct i2c_client *client, int operation,
	int ctl_name, int *nrels_mag, long *results)
{
	PTW9906DATA data = client->data;
	
//	printk("<1>TW9906ProcRead():i2c_client = %p\n", client);
	if (operation == SENSORS_PROC_REAL_INFO)
		*nrels_mag = 0;
	else if (operation == SENSORS_PROC_REAL_READ) 
	{
		TW9906UpdateClient(client);
		results[0] = data->wRead;
		*nrels_mag = 1;
	}  
}

void TW9906ProcWrite(struct i2c_client *client, int operation,
		    int ctl_name, int *nrels_mag, long *results)
{
	PTW9906DATA	data = client->data;
	
//	printk("<1>TW9906ProcWrite() be called\n");
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
			i2c_smbus_write_byte_data(client, data->wReg, data->wWrite);
		}
		
	}
}

static void HWResetSensor(struct i2c_client *client)
{	
	int k;
	i2c_smbus_write_byte_data(client, TW9906REG_ACNTL, 0x80);
	for (k = 0; k< sizeof(DefaultSetting)/sizeof(DefaultSetting[0]); k++)
		i2c_smbus_write_byte_data(client, DefaultSetting[k].uRegister, 
			DefaultSetting[k].uValue);
}

static int TW9906AttachAdapter(struct i2c_adapter *adapter)
{
	int rc;
	rc=i2c_detect(adapter, &addr_data, TW9906Detect);
	if(rc<0)
		printk("<1>call i2c_detect() failed\n");
	return rc;
}

//	This function is called by i2c_detect 
int TW9906Detect(struct i2c_adapter *adapter, int address, unsigned short flags, 
	int kind)
{
	struct i2c_client	*new_client;
	PTW9906DATA 		data;
	int 				err = 0, i, rc;
	const char			*type_name=NULL, *client_name=NULL;

//	printk("<1>TW9906Detect() be called with address=0x%02X\n", address);
	
	//	Make sure we aren't probing the ISA bus!! This is just a safety check
	//  at this moment; i2c_detect really won't call us.
#ifdef DEBUG
	if (i2c_is_isa_adapter(adapter)) 
	{
		printk("TW9906.o: TW9906Detect called for an ISA bus adapter?!?\n");
		return 0;
	}
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE | 
		I2C_FUNC_SMBUS_BYTE_DATA))
		goto ERROR0;

	//	OK. For now, we presume we have a valid client. We now create the
	//  client structure, even though we cannot fill it completely yet.
	//  But it allows us to access TW9906_{read,write}_value.
	if (!(new_client = kmalloc(sizeof(struct i2c_client)+
		 sizeof(TW9906DATA)+sizeof(SENSOR), GFP_KERNEL))) 
	{
		err = -ENOMEM;
		goto ERROR0;
	}

	g_pTW9906Data= data = (TW9906DATA *) (new_client + 1);
	g_pPLSensor = data->pSensor =(PSENSOR)(data+1);
	new_client->addr = address;
	new_client->data = data;
	new_client->adapter = adapter;
	new_client->driver = &TW9906Driver;
	new_client->flags = 0;

	//	Now, we would do the remaining detection. But the TW9906 is plainly
	//  impossible to detect! Stupid chip.
	// Determine the chip type
	if (kind <= 0) 
	{
		if (address == TW9906_I2C_IOADDR)
		{
			rc=i2c_smbus_read_byte_data(new_client, TW9906REG_VER_ID);
			if(((rc>>3)&0x1F) != TW9906_CHIP_ID)
			{
				printk("<1>Unsupported sensor; chip version = 0x%04X\n", rc);
				goto ERROR1;
			}
			kind = TW9906;
			type_name = "TW9906";
			client_name = "TW9906 chip";
		}
		else 
			goto ERROR1;
	}
	else if (kind == TW9906) 
	{
		type_name = "TW9906";
		client_name = "TW9906 chip";
	} 
	else 
	{
#ifdef DEBUG
		printk("TW9906.o: Internal error: unknown kind (%d)?!?", kind);
#endif
		goto ERROR1;
	}

	// Fill in the remaining client fields and put it into the global list 
	strcpy(new_client->name, client_name);

	new_client->id = TW9906_id++;
	data->cValid = 0;
	init_MUTEX(&data->UpdateLock);

	// Tell the I2C layer a new client has arrived
	if ((err = i2c_attach_client(new_client)))
		goto ERROR3;

	// Register a new directory entry with module sensors
	if ((i = i2c_register_entry(new_client, type_name, 
		TW9906_dir_table_template)) < 0) 
	{
		err = i;
		goto ERROR4;
	}
	data->nSysCtlID = i;

	//Initialize the TW9906D chip
    TW9906InitClient(new_client);
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


static int TW9906DetachClient(struct i2c_client *client)
{
	int err;

	i2c_deregister_entry(((PTW9906DATA) (client->data))-> nSysCtlID);
	if ((err = i2c_detach_client(client))) 
	{
		printk("TW9906.o: Client deregistration failed, client not detached.\n");
		return err;
	}
	kfree(client);
	return 0;

}
/* Called when we have found a new TW9906. */

static void TW9906InitClient(struct i2c_client *client)
{
	PTW9906DATA 	data = client->data;
//	int				i, j, k;
//	printk("<1>TW9906InitClient() be called\n");
	
	data->pSensor->nType		= TYPE_TVDECODER;	
	data->pSensor->pI2CClient	= client;
    data->pSensor->szShortName	= TW9906_SHORT_NAME;
    data->pSensor->szFullName	= TW9906_FULL_NAME;
    data->pSensor->szVersion	= TW9906_VERSION;
	data->pSensor->PrivateData	= data;
	data->pSensor->Restart		= TW9906Restart;
	data->pSensor->SetFormat	= TW9906SetFormat;
	data->pSensor->GetFormat	= TW9906GetFormat;
	data->pSensor->SetSync		= TW9906SetSync;
	data->pSensor->GetSync		= TW9906GetSync;
//#ifdef __MEDIA_PUMP__
	data->pSensor->SetInput		= TW9906SetInput;
	data->pSensor->GetInput		= TW9906GetInput;
	data->pSensor->SetVSTD		= TW9906SetVSTD;
	data->pSensor->GetVSTD		= TW9906GetVSTD;
	data->pSensor->ShowStatus	= TW9906ShowStatus;
//#endif	
	data->pSensor->SyncInfo.nHSyncStart		= 0;
	data->pSensor->SyncInfo.nVSyncStart		= 0;
	data->pSensor->SyncInfo.byITUBT656		= 0;    /* embedded sync */
	data->pSensor->SyncInfo.byHRefSelect	= 1;
	data->pSensor->SyncInfo.nHSyncPhase		= 0;
	data->pSensor->SyncInfo.byHSyncState	= 0;
	data->pSensor->SyncInfo.byVSyncState	= 0;
	data->pSensor->SyncInfo.byValidEnable	= 0;
	data->pSensor->SyncInfo.byValidState	= 0;

//#ifdef __MEDIA_PUMP__
	data->pSensor->uVSTDCapabilities = VSTD_CAP_NTSC_M | VSTD_STD_PAL | VSTD_STD_SECAM | 
		VSTD_STD_NTSC_44 | VSTD_STD_PAL_M | VSTD_STD_PAL_CN | VSTD_STD_PAL_60 |
		VSTD_CAP_AUTODETC;
	data->pSensor->uInputCapabilities= VIN_CAP_COMPOSITE | VIN_CAP_SVIDEO | 
		VIN_CAP_COMPONENT;
	data->pSensor->uCVFmtCapabilities= VIN_CAP_CVMFT_480I | VIN_CAP_CVMFT_576I |
		VIN_CAP_CVMFT_480P | VIN_CAP_CVMFT_576P | VIN_CAP_CVMFT_AUTO;
//#endif	
	
	data->DevFS = devfs_register (NULL, "sensor", 
		DEVFS_FL_AUTO_DEVNUM, 0, 0,	//	auto major and minor number
		S_IFCHR | S_IWUSR | S_IRUSR, &SensorFOPS, NULL);
	HWResetSensor(client);
//	for (k = 0; k< sizeof(DefaultSetting)/sizeof(DefaultSetting[0]); k++)
//		i2c_smbus_write_byte_data(client, DefaultSetting[k].uRegister, 
//			DefaultSetting[k].uValue);
	printk("<1>Video deocder %s is registered\n", data->pSensor->szShortName);
//	data->byWrite = TW9906_INIT;
//	i2c_smbus_write_byte(client, data->byWrite);
}


static void TW9906UpdateClient(struct i2c_client *client)
{
	TW9906DATA *data = client->data;

	down(&data->UpdateLock);
	if ((jiffies - data->uLastUpdated > 5*HZ) ||
	    (jiffies < data->uLastUpdated) || !data->cValid) 
	{
#ifdef DEBUG
		printk("Starting TW9906 update\n");
#endif
		data->wRead = i2c_smbus_read_byte(client); 
		data->uLastUpdated = jiffies;
		data->cValid = 1;
	}
	up(&data->UpdateLock);
}

static int __init sm_TW9906_init(void)
{
	printk("<1>TW9906.o version %s (%s)\n", LM_VERSION, LM_DATE);
	return i2c_add_driver(&TW9906Driver);
}

static void __exit sm_TW9906_exit(void)
{
	i2c_del_driver(&TW9906Driver);
}
//
//===========================================================================

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
//===========================================================================
//

static int SensorIOCTL(struct inode *pinode, struct file *pfile, 
	unsigned int cmd, unsigned long arg)
{

	struct i2c_client	*pClient=g_pPLSensor->pI2CClient;
	REGISTERINFO		reginfo;
	
	switch (cmd)
	{
		case PLSENSOR_WRITE_REGISTER:
			copy_from_user((void*)&reginfo , (void*)arg, sizeof(REGISTERINFO));
			i2c_smbus_write_byte_data(pClient, reginfo.uRegister, reginfo.uValue);
			break;
		case PLSENSOR_READ_REGISTER:
			copy_from_user((void*)&reginfo , (void*)arg, sizeof(REGISTERINFO));
			reginfo.uValue=i2c_smbus_read_byte_data(pClient, reginfo.uRegister);
			copy_to_user((void*)arg, (void*)&reginfo, sizeof(REGISTERINFO));
			break;
		case PLSENSOR_WRRD_REGISTER:
			copy_from_user((void*)&reginfo , (void*)arg, sizeof(REGISTERINFO));
			i2c_smbus_write_byte_data(pClient, reginfo.uRegister, reginfo.uValue);
			reginfo.uValue=i2c_smbus_read_byte_data(pClient, reginfo.uRegister);
			copy_to_user((void*)arg, (void*)&reginfo, sizeof(REGISTERINFO));
			break;
	}
	return 0;
}

//
//===========================================================================

//===========================================================================
//
static int TW9906SetSync(PSENSOR pSensor)
{
	unsigned char	value;
	pSensor->SyncInfo.byITUBT656=1;
	pSensor->SyncInfo.byHRefSelect=0;	
//	if (pSensor->SyncInfo.byITUBT656)
//	{
		//	OPFORM[7]::MODE <-- 1
		//	OPFORM[6]::LEN  <-- 0
		value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_OPFORM);
		value |= 0x80;	//	set ITU-R.656
		value &= 0xBF;	//	set 8 bit 4:2:2
		i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_OPFORM, value);
		
		//	VBICNTL[4]::HA_EN <-- 1
		value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_VBICNTL);
		value |= 0x10;
		i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_VBICNTL, value);
		
		//	DID[5]::VIPCFG <-- 1	
		value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_DID);
		value |= 0x20;
		i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_DID, value);
		
		//	VVBI[4]::NTSC656 <-- 1 for NTSC or 0 for PAL
		value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_VVBI);
		if(pSensor->VideoStandard.uFrameLines==525)
			value |= 0x10;
		else if(pSensor->VideoStandard.uFrameLines==625)
			value &= 0xEF;
		i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_VVBI, value);
//	}
//	else
//	{
//		value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_OPFORM);
//		value &= 0x7F;	//	set ITU-R.601
//		i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_OPFORM, value);
//	}
	pSensor->SyncInfo.nHSyncStart  = 0;
	pSensor->SyncInfo.nVSyncStart  = 0;
	pSensor->SyncInfo.nHSyncPhase  = 0;
	
	value = 0x00;
	pSensor->SyncInfo.byHSyncState = 0;
	value |= 1<<3;
	pSensor->SyncInfo.byVSyncState = 0;
	value |= 1<<7;
	i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_OCNTL1, value);
	
	pSensor->SyncInfo.byValidEnable= 1;
	pSensor->SyncInfo.byValidState = 1;
	return 0;	
}

static int TW9906GetSync(PSENSOR pSensor)
{
	unsigned char	value;
	
	value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_OPFORM);
	pSensor->SyncInfo.byITUBT656=(value&0x80)?1:0;
	pSensor->SyncInfo.nHSyncStart  = 0;
	pSensor->SyncInfo.nVSyncStart  = 0;
	pSensor->SyncInfo.nHSyncPhase  = 0;
	value=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_OCNTL1);
	pSensor->SyncInfo.byHSyncState = (value & (1<<3))?0:1;
	pSensor->SyncInfo.byVSyncState = (value & (1<<7))?0:1;
	pSensor->SyncInfo.byValidEnable= 1;
	pSensor->SyncInfo.byValidState = 1;
	return 0;	
}

static int TW9906SetFormat(PSENSOR pSensor)
{
    int 	rc=0;
//	int		nTotalWidth, nTotalHeight;
	int		nHActive, nVActive;
	int		nHDelay, nVDelay;
	int		nHScale, nVScale;
	
	// fixed 27MHz, no SQ output
	//	set output
	nVDelay		=(pSensor->VideoStandard.uFrameLines>>1)-(pSensor->Format.nCropHeight>>1);
	nVActive	=pSensor->Format.nCropHeight>>1;
	nHActive	=pSensor->Format.nCropWidth;
	nHDelay		=16;
	nHScale		=0x0100;
	nVScale		=0x0100;
	pSensor->Format.nFPS=30;
	
//	printk("<1> vdelay=%d, vactive=%d, hdelay=%d, hactive=%d\n", 
//		nVDelay, nVActive, nHDelay, nHActive);
/*	
	if(pSensor->SyncInfo.byITUBT656)
	{
		if(pSensor->VideoStandard.uFrameLines==525)
		{
			if(pSensor->Format.nCropWidth==720 && 
				pSensor->Format.nCropHeight == 480)
			{	
				nTotalWidth	=858;
				nTotalHeight=525;
				nVDelay  	=22;
				nVActive 	=240;
				nHActive 	=704;
				nHDelay		=16;
				nHScale		=0x0100;
				nVScale		=0x0100;
			}
		}	
		else if(pSensor->VideoStandard.uFrameLines==625)
		{
			if(pSensor->Format.nSrcWidth==720 && 
				pSensor->Format.nSrcHeight == 576)
			{	
				nTotalWidth	=864;
				nTotalHeight=625;
				nHActive	=720;
				nVDelay		=24;
				nVActive	=288;
				nHDelay		=16;
				nHScale		=0x0100;
				nVScale		=0x0100;
			}
		}
	}
	else
	{	//	output with ITUBT601
		if(pSensor->VideoStandard.uFrameLines==525)
		{
			if(pSensor->Format.nSrcWidth==720 && 
				pSensor->Format.nSrcHeight == 480)
			{	
				nTotalWidth	=858;
				nTotalHeight=525;
				nVDelay  	=(nTotalHeight-480)>>1;
				nVActive 	=240;
				nHActive 	=720;
				nHDelay		=16;
				nHScale		=0x0100;
				nVScale		=0x0100;
			}
			else if(pSensor->Format.nSrcWidth==360 && 
				pSensor->Format.nSrcHeight == 240)
			{
				nTotalWidth	=429;
				nTotalHeight=262;
				nHActive	=360;
				nVDelay		=(nTotalHeight-240)>>1;
				nVActive	=120;
				nHDelay		=16;
				nHScale		=0x0200;
				nVScale		=0x0200;
			}
			else if(pSensor->Format.nSrcWidth==180 && 
				pSensor->Format.nSrcHeight == 120)
			{
				nTotalWidth	=214;
				nTotalHeight=131;
				nHActive	=180;
				nVDelay		=(nTotalHeight-120)>>1;
				nVActive	=60;
				nHDelay		=16;
				nHScale		=0x0400;
				nVScale		=0x0400;
			}
		}	
		else if(pSensor->VideoStandard.uFrameLines==625)
		{
			
			if(pSensor->Format.nSrcWidth==720 && 
				pSensor->Format.nSrcHeight == 576)
			{	
				nTotalWidth	=864;
				nTotalHeight=625;
				nHActive	=720;
				nVDelay		=(nTotalHeight-576)>>1;
				nVActive	=288;
				nHScale		=0x0100;
				nVScale		=0x0100;
			}
			else if(pSensor->Format.nSrcWidth==360 && 
				pSensor->Format.nSrcHeight == 288)
			{
				nTotalWidth	=432;
				nTotalHeight=312;
				nHActive	=360;
				nVDelay		=(nTotalHeight-288)>>1;
				nVActive	=144;
				nHScale		=0x0200;
				nVScale		=0x0200;
			}
			else if(pSensor->Format.nSrcWidth==180 && 
				pSensor->Format.nSrcHeight == 144)
			{
				nTotalWidth	=216;
				nTotalHeight=156;
				nHActive	=180;
				nVDelay		=(nTotalHeight-144)>>1;
				nVActive	=72;
				nHScale		=0x0400;
				nVScale		=0x0400;
			}
		}
	}
*/	
    rc = i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_VDELAY_LO, 
		(nVDelay & 0xFF) );
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_VACTIVE_LO, 
		(nVActive & 0xFF) );
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_HDELAY_LO, 
		(nHDelay & 0xFF) );
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_HACTIVE_LO,  
		(nHActive & 0xFF) );
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_CROP_HI,  
		((nVDelay & 0x300) >> 2 ) | ((nVActive & 0x300) >> 4) |
		((nHDelay & 0x300) >> 6)  | ((nHActive & 0x300) >> 8) );
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_VSCALE_LO, 
		(nVScale & 0xFF) );
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_HSCALE_LO,  
		(nHScale & 0xFF));
    rc += i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_SCALE_HI, 
		((nVScale & 0xF00) >> 4) | ((nHScale & 0xF00) >> 8) );
    return 0;
}

static int TW9906GetFormat(PSENSOR pSensor)
{
//	unsigned short wData;
	return 0;	

}

static void TW9906Restart(PSENSOR pSensor)
{
//	struct i2c_clien	*pClient=pSensor->pI2CClient;
//	int					i, j, k;
	
	HWResetSensor(pSensor->pI2CClient);
//	for (k = 0; k< sizeof(DefaultSetting)/sizeof(DefaultSetting[0]); k++)
//		i2c_smbus_write_byte_data(pClient, 
//			DefaultSetting[k].uReg, 
//			DefaultSetting[k].uVal);
}

//#ifdef __MEDIA_PUMP__

//===========================================================================
//	For PL1029 Video decoder layout 
//	Joe
//***************************************************************************
// Composite----> MUX0 (Y+U+V)
//***************************************************************************
// S-Video---+--> MUX1 (Y)
//           |
//           +--> CIN0 (C=U+V)
//***************************************************************************
// Component-+--> MUX3 (Y)
//           |
//           +--> CIN1 (Pb=U)
//           |
//           +--> VIN0 (Pr=V)
//***************************************************************************
//===========================================================================
static int SetInputSource(PSENSOR pSensor)
{
	int 			rc=0;
//	unsigned char	value;
	//	TODO :  power down chroma ADC & V channel ADC while not be used
	//			in s-video and component
	switch(pSensor->uInput)
	{
		case VIN_COMPOSITE:
			// Select Composite Video
			rc = i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_INFORM, 
				TW9906COMPOSITE_SEL); 
			pSensor->Format.nInterlaced=1;
			break;
		case VIN_SVIDEO:
			// Select S-Video
			rc = i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_INFORM, 
				TW9906SVIDEO_SEL);
			pSensor->Format.nInterlaced=1;
			break;
		case VIN_COMPONENT:
			//	 Select YPbPr 
			rc = i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_INFORM, 
				TW9906COMPONENT_SEL); 
			if(!pSensor->uCVFmtCapabilities&(1<<pSensor->uCVFmt))
				pSensor->uCVFmt=VIN_CVFMT_480I;	// TODO : should be set to default value
			if(pSensor->uCVFmt==VIN_CVFMT_480I || pSensor->uCVFmt==VIN_CVFMT_576I)
				pSensor->Format.nInterlaced=1;
			else if(pSensor->uCVFmt==VIN_CVFMT_480P || pSensor->uCVFmt==VIN_CVFMT_576P)
				pSensor->Format.nInterlaced=0;
			rc = i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_CVFMT, 
				(pSensor->uCVFmt&0x0F)); 			
			break;
    }
	if (rc < 0)
	{
		printk("<1>[TVDEC] : failed to select input format\n");
		rc=-EINVAL;
	}
	return rc;
}

static int TW9906SetInput(PSENSOR pSensor)
{
	if(!(pSensor->uInputCapabilities & (1<<pSensor->uInput)))
		pSensor->uInput=VIN_COMPOSITE;	// TODO: should be set to default value
	return SetInputSource(pSensor);
}

static int TW9906GetInput(PSENSOR pSensor)
{
	unsigned char	byte;
	
	byte=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_INFORM);
	pSensor->uInput=(byte>>4)&0x03;
	if(pSensor->uInput==VIN_COMPONENT)
	{
		byte=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_CVFMT);
		pSensor->uCVFmt=(byte>>4) & 0x0F;
		if(pSensor->uCVFmt==VIN_CVFMT_480I || pSensor->uCVFmt==VIN_CVFMT_576I)
			pSensor->Format.nInterlaced=1;
		else if(pSensor->uCVFmt==VIN_CVFMT_480P || pSensor->uCVFmt==VIN_CVFMT_576P)
			pSensor->Format.nInterlaced=0;
	}
	return 0;
}

static void SetVideoStandard(PSENSOR pSensor)
{
	pSensor->Format.nInterlaced=1;
	switch(pSensor->uVSTD)
	{
		case VSTD_STD_NTSC_M:
			pSensor->VideoStandard.FrameRate.uNumerator=1001;
			pSensor->VideoStandard.FrameRate.uDenominator=30000;
			pSensor->VideoStandard.uFrameLines=525;
			pSensor->VideoStandard.uFields=60;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_NTSC;
			pSensor->VideoStandard.ColourStandardData.NTSC.uColourSubcarrier=
				VSTD_COLOR_SUBC_NTSC;
			break;
		case VSTD_STD_PAL:
			pSensor->VideoStandard.FrameRate.uNumerator=1;
			pSensor->VideoStandard.FrameRate.uDenominator=25;
			pSensor->VideoStandard.uFrameLines=625;
			pSensor->VideoStandard.uFields=50;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_PAL;
			pSensor->VideoStandard.ColourStandardData.PAL.uColourSubcarrier=
				VSTD_COLOR_SUBC_PAL;
			break;
		case VSTD_STD_SECAM:
			pSensor->VideoStandard.FrameRate.uNumerator=1;
			pSensor->VideoStandard.FrameRate.uDenominator=25;
			pSensor->VideoStandard.uFrameLines=625;
			pSensor->VideoStandard.uFields=50;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_SECAM;
			pSensor->VideoStandard.ColourStandardData.SECAM.uf0b=
				VSTD_COLOR_SUBC_SECAMB;
			pSensor->VideoStandard.ColourStandardData.SECAM.uf0r=
				VSTD_COLOR_SUBC_SECAMR;
			break;
		case VSTD_STD_NTSC_44:
			pSensor->VideoStandard.FrameRate.uNumerator=1001;
			pSensor->VideoStandard.FrameRate.uDenominator=30000;
			pSensor->VideoStandard.uFrameLines=525;
			pSensor->VideoStandard.uFields=60;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_NTSC;
			pSensor->VideoStandard.ColourStandardData.NTSC.uColourSubcarrier=
				VSTD_COLOR_SUBC_PAL;
			break;
		case VSTD_STD_PAL_M:
			pSensor->VideoStandard.FrameRate.uNumerator=1001;
			pSensor->VideoStandard.FrameRate.uDenominator=30000;
			pSensor->VideoStandard.uFrameLines=525;
			pSensor->VideoStandard.uFields=60;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_PAL;
			pSensor->VideoStandard.ColourStandardData.PAL.uColourSubcarrier=
				VSTD_COLOR_SUBC_PAL_M;
			break;
		case VSTD_STD_PAL_CN:
			pSensor->VideoStandard.FrameRate.uNumerator=1;
			pSensor->VideoStandard.FrameRate.uDenominator=25;
			pSensor->VideoStandard.uFrameLines=625;
			pSensor->VideoStandard.uFields=50;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_PAL;
			pSensor->VideoStandard.ColourStandardData.PAL.uColourSubcarrier=
				VSTD_COLOR_SUBC_PAL_CN;
			break;
		case VSTD_STD_PAL_60:
			pSensor->VideoStandard.FrameRate.uNumerator=1001;
			pSensor->VideoStandard.FrameRate.uDenominator=30000;
			pSensor->VideoStandard.uFrameLines=525;
			pSensor->VideoStandard.uFields=60;
			pSensor->VideoStandard.uColourStandard=VSTD_COLOR_STD_PAL;
			pSensor->VideoStandard.ColourStandardData.PAL.uColourSubcarrier=
				VSTD_COLOR_SUBC_PAL;
			break;
	}
}

static int TW9906SetVSTD(PSENSOR pSensor)
{
	unsigned char	byte;

	if(!(pSensor->uVSTDCapabilities & (1<<pSensor->uVSTD)))
		pSensor->uVSTD=VSTD_STD_NTSC_M;	// TODO: should be set to default value
	i2c_smbus_write_byte_data(pSensor->pI2CClient, TW9906REG_SDT, 
		(pSensor->uVSTD & 0x07));
	byte=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_SDT);
	pSensor->uVSTD=(byte>>4) & 0x07;
	SetVideoStandard(pSensor);	
	return 0;
}

static char *g_szVSTDName[8]=
{
	"NTSC(M)",
	"PAL(B,D,G,H,I)",
	"SECAM",
	"NTSC4.43",
	"PAL(M)",
	"PAL(CN)",
	"PAL60",
	"invalid"
};

static int TW9906GetVSTD(PSENSOR pSensor)
{
	unsigned char	byte;

    // Check Video Mode 
	printk("<1>Check Video Mode...\n");
    do
	{
		byte=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_SDT);
	}	while(byte & STD_ON_DETECTION);
	printk("<1>Found %s video mode\n", 
		((byte>>4)&0x07)<7?g_szVSTDName[(byte>>4)&0x07]:g_szVSTDName[7]);
	
	if(pSensor->uVSTD==((byte>>4) & 0x07))
		return 0;
	
	pSensor->uVSTD=(byte>>4) & 0x07;
	SetVideoStandard(pSensor);	
    return 0;
}

static int TW9906ShowStatus(PSENSOR pSensor)
{
	unsigned char	byte;
	int				rc=0;
	
	byte=i2c_smbus_read_byte_data(pSensor->pI2CClient, TW9906REG_STATUS1);
	if(!(byte & 0x80))
	{
		printk("<1>TW9906 : video detected\n");
		printk("<1>\tHLOCK is %s\n", byte&0x40?"locked":"not locked");
		printk("<1>\tSLOCK is %s\n", byte&0x20?"locked":"not locked");
		printk("<1>\tVLOCK is %s\n", byte&0x08?"locked":"not locked");
		printk("<1>\t%s field is decoded\n", byte&0x10?"even":"odd");
		printk("<1>\t%s\n", byte&0x02?"no color burst detected":"color burst detected");
		printk("<1>\t%d Hz source\n", byte&0x01?50:60);
	}
	else
	{
		printk("<1>TW9906 : video not present\n");
		rc=-EFAULT;
	}
	return rc;
}
//#endif
//
//===========================================================================

MODULE_AUTHOR("Chen Chao Yi<joe-chen@prolific.com.tw>");
MODULE_DESCRIPTION("TW9906 driver");
EXPORT_SYMBOL(g_pPLSensor);
module_init(sm_TW9906_init);
module_exit(sm_TW9906_exit);


