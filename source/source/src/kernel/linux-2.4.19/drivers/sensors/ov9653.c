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
#include "ov9653.h"

#define		LM_DATE		"20060424"
#define		LM_VERSION	"1.0.0.0"
MODULE_LICENSE("GPL");

// Addresses to scan 
static unsigned short normal_i2c[] = { OV9653_I2C_IOADDR, SENSORS_I2C_END };
static unsigned short normal_i2c_range[] = { SENSORS_I2C_END };
static unsigned int normal_isa[] = { SENSORS_ISA_END };
static unsigned int normal_isa_range[] = { SENSORS_ISA_END };

// Insmod parameters 
SENSORS_INSMOD_1(OV9653);


static int OV9653AttachAdapter(struct i2c_adapter *adapter);
static int OV9653Detect(struct i2c_adapter *adapter, int address, unsigned short flags, int kind);
static int OV9653DetachClient(struct i2c_client *client);
static void OV9653ProcRead(struct i2c_client *client, int operation, int ctl_name, int *nrels_mag, long *results);
static void OV9653ProcWrite(struct i2c_client *client, int operation, int ctl_name, int *nrels_mag, long *results);
static void OV9653UpdateClient(struct i2c_client *client);
static void OV9653InitClient(struct i2c_client *client);
static int OV9653ReadByte(struct i2c_client *client, int reg);
static int OV9653WriteByte(struct i2c_client *client, int reg, int val);
 
static int OV9653SetFormat(PSENSOR pSensor);
static int OV9653GetFormat(PSENSOR pSensor);
static int OV9653SetSync(PSENSOR pSensor);
static int OV9653GetSync(PSENSOR pSensor);
static void OV9653Restart(PSENSOR pSensor);

static int SensorOpen(struct inode *pinode, struct file *pfile);
static int SeneorRelese(struct inode *pinode, struct file *pfile);
static int SensorIOCTL(struct inode *pinode, struct file *pfile, 
	unsigned int cmd, unsigned long arg);

static const char OV9653_SHORT_NAME[]="OV9653";
static const char OV9653_FULL_NAME[]="OmniVision OV9653 SXGA Sensor";
static const char OV9653_VERSION[]="1.0.0.0";

static struct file_operations SensorFOPS=
{
	owner:		THIS_MODULE,
	open:		SensorOpen,
	release:	SeneorRelese,
	ioctl:		SensorIOCTL,
};
	
//===========================================================================
//	yuv vga
REGISTERINFO OV9653YUVVGADefault[]=
{
	{ OV9653REG_GAIN,	0x00 },	//	0x00
	{ OV9653REG_BLUE,	0x80 },	//	0x80
	{ OV9653REG_RED,	0x80 },	//	0x80
	{ OV9653REG_VREF,	0x12 },
	{ OV9653REG_COM1,	0x00 },	
//	{ OV9653REG_BAVE,	0x00 },	//	auto
//	{ OV9653REG_GEAVE,	0x00 },	//	auto
//	{ OV9653REG_RAVE,	0x00 },	//	auto
	{ OV9653REG_COM2,	0x01 },	// bit[1:0] => 00: 1x, 01: 2x, 10: 2x, 11: 4x
	{ OV9653REG_COM3,	0x00 },
	{ OV9653REG_COM4,	0x00 },
	{ OV9653REG_COM5,	0x81 },
	{ OV9653REG_COM6,	0x43 },
//	{ OV9653REG_AECH,	0x40 },	//	auto
	{ OV9653REG_CLKRC,	0x00 },
	{ OV9653REG_COM7,	0x00 },
	{ OV9653REG_COM8,	0x8F },
	{ OV9653REG_COM9,	0x4A },
	{ OV9653REG_COM10,	0x00 },
	{ OV9653REG_HSTART,	0x1A },
	{ OV9653REG_HSTOP,	0xBA },
	{ OV9653REG_VSTART,	0x01 },
	{ OV9653REG_VSTOP,	0x81 },
	{ OV9653REG_PSHFT,	0x00 },
	{ OV9653REG_MVFP,	0x00 },
	{ OV9653REG_LAEC,	0x00 },
	{ OV9653REG_BOS,	0x80 },
	{ OV9653REG_GBOS,	0x80 },
	{ OV9653REG_GROS,	0x80 },
	{ OV9653REG_ROS,	0x80 },
	{ OV9653REG_AEW,	0x78 },
	{ OV9653REG_AEB,	0x68 },
	{ OV9653REG_VPT,	0xD4 },
	{ OV9653REG_BBIAS,	0x80 },
	{ OV9653REG_GbBIAS,	0x80 },
	{ OV9653REG_GrCOM,	0x00 },
	{ OV9653REG_EXHCH,	0x00 },
	{ OV9653REG_EXHCL,	0x00 },
	{ OV9653REG_RBIAS,	0x80 },
	{ OV9653REG_ADVFL,	0x00 },
	{ OV9653REG_ADVFH,	0x00 },
	{ OV9653REG_YAVE,	0x00 },
	{ OV9653REG_HSYST,	0x08 },
	{ OV9653REG_HSYEN,	0x30 },
	{ OV9653REG_HREF,	0xA4 },
	{ OV9653REG_CHLF,	0x00 },
	{ OV9653REG_ARBLM,	0x03 },
	{ OV9653REG_ADC,	0x04 },
	{ OV9653REG_ACOM,	0x12 },
	{ OV9653REG_OFON,	0x00 },
	{ OV9653REG_TSLB,	0x0C },	//0x0c
	{ OV9653REG_COM11,	0x80 },
	{ OV9653REG_COM12,	0x40 },
	{ OV9653REG_COM13,	0x99 },
	{ OV9653REG_COM14,	0x0E },
	{ OV9653REG_EDGE,	0x88 },
	{ OV9653REG_COM15,	0xC0 },
	{ OV9653REG_COM16,	0x10 },
	{ OV9653REG_COM17,	0x08 },
	{ OV9653REG_MANU,	0x80 },
	{ OV9653REG_MANV,	0x80 },
	{ OV9653REG_HV,		0x00 },
	{ OV9653REG_MBD,	0x00 },
//	{ OV9653REG_DBLV,	0x3A },
	{ OV9653REG_COM21,	0x04 },
	{ OV9653REG_COM22,	0x00 },
	{ OV9653REG_COM23,	0x00 },
	{ OV9653REG_COM24,	0x00 },
	{ OV9653REG_DBLC1,	0x0F },
	{ OV9653REG_DBLC_B,	0x00 },
	{ OV9653REG_DBLC_R,	0x00 },
	{ OV9653REG_DM_LML,	0x00 },
	{ OV9653REG_DM_LMH,	0x00 },
	{ OV9653REG_LCCFB,	0x00 },
	{ OV9653REG_LCCFR,	0x00 },
	{ OV9653REG_DBLC_Gb,0x00 },
	{ OV9653REG_DBLC_Gr,0x00 },
	{ OV9653REG_AECHM,	0x40 },
	{ OV9653REG_COM25,	0x00 },
	{ OV9653REG_COM26,	0x00 },
	{ OV9653REG_G_GAIN,	0x80 },
	{ OV9653REG_VGA_ST,	0x14 }
};

REGISTERINFO OV9653YUVVGAOptimalSet[]=
{
	{ OV9653REG_COM7,	0x80 },
	{ OV9653REG_CLKRC,	0x80 },
//	{ OV9653REG_DBLV,	0x4A },
	{ OV9653REG_MBD,	0x3E },
	{ OV9653REG_COM11,	0x09 },
	{ OV9653REG_COM8,	0xE0 },
	{ OV9653REG_BLUE,	0x80 },
	{ OV9653REG_RED,	0x80 },
	{ OV9653REG_GAIN,	0x00 },
	{ OV9653REG_AECH,	0x00 },
	{ OV9653REG_COM8,	0xE5 },
	//	Take a break	
	{ OV9653REG_OFON,	0x43 },
	{ OV9653REG_ACOM,	0x12 },
	{ OV9653REG_ADC,	0x00 },
	{ 0x35,				0x91 },
	{ OV9653REG_COM5,	0xA0 },
	{ OV9653REG_MVFP,	0x04 },
	//	Take a break	
	{ 0xA8,				0x80 },
	{ OV9653REG_COM7,	0x40 },
	{ OV9653REG_COM1,	0x00 },
	{ OV9653REG_COM3,	0x04 },
	{ OV9653REG_COM4,	0x80 },
	{ OV9653REG_HSTOP,	0xC6 },
	{ OV9653REG_HSTART,	0x26 },
	{ OV9653REG_HREF,	0xAD },
	{ OV9653REG_VREF,	0x00 },
	{ OV9653REG_VSTOP,	0x3D },
	{ OV9653REG_VSTART,	0x01 },
	{ OV9653REG_EDGE,	0xA6 },
	{ OV9653REG_COM9,	0x4E },
	{ OV9653REG_COM10,	0x02 },
	{ OV9653REG_COM16,	0x02 },
	{ OV9653REG_COM17,	0x08 },
	//	Take a break	
	{ OV9653REG_PSHFT,	0x00 },
	{ 0x16,				0x06 },
	{ OV9653REG_CHLF,	0xE2 },
	{ OV9653REG_ARBLM,	0xBF },
	{ 0x96,				0x04 },
	{ OV9653REG_TSLB,	0x0C },
	{ OV9653REG_COM24,	0x00 },
	//	Take a break	
	{ OV9653REG_COM12,	0x70 },
	{ OV9653REG_COM21,	0x06 },
	{ 0x94,				0x88 },
	{ 0x95,				0x88 },
	{ OV9653REG_COM15,	0xC1 },
	{ OV9653REG_GrCOM,	0x3F },
	{ OV9653REG_COM6,	0x42 },
	//	Take a break	
	{ OV9653REG_COM13,	0x92 },
	{ OV9653REG_HV,		0x40 },
	{ 0x5C,				0xB9 },	//??
	{ 0x5D,				0x96 },
	{ 0x5E,				0x10 },
	{ 0x59,				0xC0 },
	{ 0x5A,				0xAF },
	{ 0x5B,				0x55 },
	{ 0x43,				0xF0 },
	{ 0x44,				0x10 },
	{ 0x45,				0x68 },
	{ 0x46,				0x96 },
	{ 0x47,				0x60 },
	{ 0x48,				0x80 },
	{ 0x5F,				0xE0 },
	{ 0x60,				0x8C },
	{ 0x61,				0x20 },
	{ OV9653REG_COM26,	0xD9 },
	{ OV9653REG_COM25,	0x74 },
	{ OV9653REG_COM23,	0x02 },
	{ OV9653REG_COM8,	0xE7 },
	//	Set Matrix	
	{ OV9653REG_MTX1, 	0x3A },
	{ OV9653REG_MTX2, 	0x3D },
	{ OV9653REG_MTX3, 	0x03 },
	{ OV9653REG_MTX4, 	0x12 },
	{ OV9653REG_MTX5, 	0x26 },
	{ OV9653REG_MTX6, 	0x38 },
	{ OV9653REG_MTX7, 	0x40 },
	{ OV9653REG_MTX8, 	0x40 },
	{ OV9653REG_MTX9, 	0x40 },
	{ OV9653REG_MTXS, 	0x0D },
	//	Take a break	
	{ OV9653REG_COM22,	0x23 },
	{ OV9653REG_COM14,	0x02 },
	{ 0xA9,				0xB8 },
	{ 0xAA,				0x92 },
	{ 0xAB,				0x0A },
	//	Take a break	
	{ OV9653REG_DBLC1,	0xDF },
	{ OV9653REG_DBLC_B,	0x00 },
	{ OV9653REG_DBLC_R,	0x00 },
	{ OV9653REG_DBLC_Gb,0x00 },
	{ OV9653REG_DBLC_Gr,0x00 },
	{ OV9653REG_TSLB,	0x0E },
	//	Take a break	
	{ OV9653REG_AEW,	0x74 },
	{ OV9653REG_AEB,	0x68 },
	{ OV9653REG_VPT,	0xB4 },
	//	Take a break	
	{ OV9653REG_EXHCH,	0x00 },
	{ OV9653REG_EXHCL,	0x00 },
	{ OV9653REG_ADVFL,	0x00 },
	{ OV9653REG_ADVFH,	0x00 },
	//	Set Gamma curve	
	{ 0x6C,				0x40 },
	{ 0x6D,				0x30 },
	{ 0x6E,				0x4B },
	{ 0x6F,				0x60 },
	{ 0x70,				0x70 },
	{ 0x71,				0x70 },
	{ 0x72,				0x70 },
	{ 0x73,				0x70 },
	{ 0x74,				0x60 },
	{ 0x75,				0x60 },
	{ 0x76,				0x50 },
	{ 0x77,				0x48 },
	{ 0x78,				0x3A },
	{ 0x79,				0x2E },
	{ 0x7A,				0x28 },
	{ 0x7B,				0x22 },
	{ 0x7C,				0x04 },
	{ 0x7D,				0x07 },
	{ 0x7E,				0x10 },
	{ 0x7F,				0x28 },
	{ 0x80,				0x36 },
	{ 0x81,				0x44 },
	{ 0x82,				0x52 },
	{ 0x83,				0x60 },
	{ 0x84,				0x6C },
	{ 0x85,				0x78 },
	{ 0x86,				0x8C },
	{ 0x87,				0x9E },
	{ 0x88,				0xBB },
	{ 0x89,				0xD2 },
	{ 0x8A,				0xE6 }
};
/*
REGISTERINFO OV9653Matrix[]=
{
	{ OV9653REG_MTX1, 	0x3A },
	{ OV9653REG_MTX2, 	0x3D },
	{ OV9653REG_MTX3, 	0x03 },
	{ OV9653REG_MTX4, 	0x12 },
	{ OV9653REG_MTX5, 	0x26 },
	{ OV9653REG_MTX6, 	0x38 },
	{ OV9653REG_MTX7, 	0x40 },
	{ OV9653REG_MTX8, 	0x40 },
	{ OV9653REG_MTX9, 	0x40 },
	{ OV9653REG_MTXS, 	0x0D }
};
	
REGISTERINFO OV9653GammaCurve[]=
{
	{ 0x6C,				0x40 },
	{ 0x6D,				0x30 },
	{ 0x6E,				0x4B },
	{ 0x6F,				0x60 },
	{ 0x70,				0x70 },
	{ 0x71,				0x70 },
	{ 0x72,				0x70 },
	{ 0x73,				0x70 },
	{ 0x74,				0x60 },
	{ 0x75,				0x60 },
	{ 0x76,				0x50 },
	{ 0x77,				0x48 },
	{ 0x78,				0x3A },
	{ 0x79,				0x2E },
	{ 0x7A,				0x28 },
	{ 0x7B,				0x22 },
	{ 0x7C,				0x04 },
	{ 0x7D,				0x07 },
	{ 0x7E,				0x10 },
	{ 0x7F,				0x28 },
	{ 0x80,				0x36 },
	{ 0x81,				0x44 },
	{ 0x82,				0x52 },
	{ 0x83,				0x60 },
	{ 0x84,				0x6C },
	{ 0x85,				0x78 },
	{ 0x86,				0x8C },
	{ 0x87,				0x9E },
	{ 0x88,				0xBB },
	{ 0x89,				0xD2 },
	{ 0x8A,				0xE6 }
};
*/
//
//===========================================================================

static ctl_table OV9653_dir_table_template[] = 
{
	{
		OV9653_SYSCTL_READ, 
		"read", 
		NULL, 
		0, 
		0444, 
		NULL, 
		&i2c_proc_real,
		&i2c_sysctl_real, 
		NULL, 
		&OV9653ProcRead
	},
	{
		OV9653_SYSCTL_WRITE, 
		"write", 
		NULL, 
		0, 
		0644, 
		NULL, 
		&i2c_proc_real,
		&i2c_sysctl_real, 
		NULL, 
		&OV9653ProcWrite
	},
	{0}
};

POV9653DATA g_pOV9653Data;
PSENSOR		g_pPLSensor;
static int 	OV9653_id = 0;

static struct i2c_driver OV9653Driver=
{
	.owner		= THIS_MODULE,
	.name		= "OV9653 sensor chip driver",	//	max. 40 characters limited
	.id			= I2C_DRIVERID_OV9653,			//	usually from i2c-id.h
	.flags		= I2C_DF_NOTIFY,				//	always set this value
 	.attach_adapter	= OV9653AttachAdapter,		
	.detach_client	= OV9653DetachClient,
};


static int OV9653ReadByte(struct i2c_client *client, int reg)
{
	i2c_smbus_write_byte(client, reg);
	return i2c_smbus_read_byte(client);
}

static int OV9653WriteByte(struct i2c_client *client, int reg, int val)
{
	return i2c_smbus_write_byte_data(client, reg, val&0xFF);
}

void OV9653ProcRead(struct i2c_client *client, int operation,
	int ctl_name, int *nrels_mag, long *results)
{
	POV9653DATA data = client->data;
	
//	printk("<1>OV9653ProcRead():i2c_client = %p\n", client);
	if (operation == SENSORS_PROC_REAL_INFO)
		*nrels_mag = 0;
	else if (operation == SENSORS_PROC_REAL_READ) 
	{
		OV9653UpdateClient(client);
		results[0] = data->wRead;
		*nrels_mag = 1;
	}  
}

void OV9653ProcWrite(struct i2c_client *client, int operation,
	int ctl_name, int *nrels_mag, long *results)
{
	POV9653DATA	data = client->data;
	
//	printk("<1>OV9653ProcWrite() be called\n");
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
			OV9653WriteByte(client, data->wReg, data->wWrite);
		}
		
	}
}

static void HWResetSensor(struct i2c_client *client)
{
	int i;
	
	OV9653WriteByte(client, OV9653REG_COM7, 0x80);	// change to page 0
	OV9653WriteByte(client, OV9653REG_COM7, 0x00);	//	set reset bit [0]<--1
	for(i=0; i< sizeof(OV9653YUVVGAOptimalSet)/sizeof(OV9653YUVVGAOptimalSet[0]); i++)
		OV9653WriteByte(client, 
			OV9653YUVVGAOptimalSet[i].uRegister, 
			OV9653YUVVGAOptimalSet[i].uValue);
	printk("<1>Set %d registers value\n", i);
}

static int OV9653AttachAdapter(struct i2c_adapter *adapter)
{
	int rc;
	printk("<1>OV9653AttachAdapter() be called\n");
	rc=i2c_detect(adapter, &addr_data, OV9653Detect);
	if(rc<0)
		printk("<1>call i2c_detect() failed\n");
	return rc;
}

//	This function is called by i2c_detect 
int OV9653Detect(struct i2c_adapter *adapter, int address, unsigned short flags, int kind)
{
	struct i2c_client	*new_client;
	POV9653DATA 		data;
	int 				err = 0, i, rc;
	const char			*type_name, *client_name;

	printk("<1>OV9653Detect() be called\n");
	//	Make sure we aren't probing the ISA bus!! This is just a safety check
	//  at this moment; i2c_detect really won't call us.
#ifdef DEBUG
	if (i2c_is_isa_adapter(adapter)) 
	{
		printk("OV9653.o: OV9653Detect called for an ISA bus adapter?!?\n");
		return 0;
	}
#endif

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE | 
		I2C_FUNC_SMBUS_BYTE_DATA))
		goto ERROR0;

	//	OK. For now, we presume we have a valid client. We now create the
	//  client structure, even though we cannot fill it completely yet.
	//  But it allows us to access OV9653_{read,write}_value.
	if (!(new_client = kmalloc(sizeof(struct i2c_client)+
		 sizeof(OV9653DATA)+sizeof(SENSOR), GFP_KERNEL))) 
	{
		err = -ENOMEM;
		goto ERROR0;
	}

	g_pOV9653Data= data = (OV9653DATA *) (new_client + 1);
	g_pPLSensor = data->pSensor =(PSENSOR)(data+1);
	new_client->addr = address;
	new_client->data = data;
	new_client->adapter = adapter;
	new_client->driver = &OV9653Driver;
	new_client->flags = 0;

	//	Now, we would do the remaining detection. But the OV9653 is plainly
	//  impossible to detect! Stupid chip.
	// Determine the chip type
	if (kind <= 0) 
	{
		if( address == OV9653_I2C_IOADDR &&
			OV9653_PID==OV9653ReadByte(new_client, OV9653REG_PID) &&
			OV9653_VER==OV9653ReadByte(new_client, OV9653REG_VER))
				kind = OV9653;
		else 
		{
			printk("<1>Unknown sensor\n");
			goto ERROR1;
		}
	}
	else if (kind == OV9653) 
	{
		type_name = "OV9653";
		client_name = "OV9653 chip";
	} 
	else 
	{
#ifdef DEBUG
		printk("OV9653.o: Internal error: unknown kind (%d)?!?", kind);
#endif
		goto ERROR1;
	}

	// Fill in the remaining client fields and put it into the global list 
	strcpy(new_client->name, client_name);

	new_client->id = OV9653_id++;
	data->cValid = 0;
	init_MUTEX(&data->UpdateLock);

	// Tell the I2C layer a new client has arrived
	if ((err = i2c_attach_client(new_client)))
		goto ERROR3;

	// Register a new directory entry with module sensors
	if ((i = i2c_register_entry(new_client, type_name, 
		OV9653_dir_table_template)) < 0) 
	{
		err = i;
		goto ERROR4;
	}
	data->nSysCtlID = i;

	//Initialize the OV9653D chip
    OV9653InitClient(new_client);
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


static int OV9653DetachClient(struct i2c_client *client)
{
	int err;

//	printk("<1>OV9653DetachClient() be called\n");
	i2c_deregister_entry(((POV9653DATA) (client->data))-> nSysCtlID);

	if ((err = i2c_detach_client(client))) 
	{
		printk("OV9653.o: Client deregistration failed, client not detached.\n");
		return err;
	}
	kfree(client);
	return 0;

}
/* Called when we have found a new OV9653. */

static void OV9653InitClient(struct i2c_client *client)
{
	POV9653DATA 	data = client->data;
	int				i;
	
	data->pSensor->nType		= TYPE_SENSOR;	
	data->pSensor->pI2CClient	= client;
    data->pSensor->szShortName	= OV9653_SHORT_NAME;
    data->pSensor->szFullName	= OV9653_FULL_NAME;
    data->pSensor->szVersion	= OV9653_VERSION;
	data->pSensor->PrivateData	= data;
	data->pSensor->Restart		= OV9653Restart;
	data->pSensor->SetFormat	= OV9653SetFormat;
	data->pSensor->GetFormat	= OV9653GetFormat;
	data->pSensor->SetSync		= OV9653SetSync;
	data->pSensor->GetSync		= OV9653GetSync;
	
	data->pSensor->SyncInfo.nHSyncStart		= 0;
	data->pSensor->SyncInfo.nVSyncStart		= 0;
	data->pSensor->SyncInfo.byITUBT656		= 1;    /* embedded sync */
	data->pSensor->SyncInfo.byHRefSelect	= 0;
	data->pSensor->SyncInfo.nHSyncPhase		= 0;
	data->pSensor->SyncInfo.byHSyncState	= 0;
	data->pSensor->SyncInfo.byVSyncState	= 0;
	data->pSensor->SyncInfo.byValidEnable	= 0;
	data->pSensor->SyncInfo.byValidState	= 0;
	
	data->pSensor->Format.nIsVarioPixel		= 0;
	data->pSensor->Format.nInterlaced		= 0;

	data->DevFS = devfs_register (NULL, "sensor", 
		DEVFS_FL_AUTO_DEVNUM, 0, 0,	//	auto major and minor number
		S_IFCHR | S_IWUSR | S_IRUSR, &SensorFOPS, NULL);
	HWResetSensor(client);
	printk("<1>CMOS sensor %s is avaliable now\n", data->pSensor->szShortName);
	for(i=0; i< sizeof(OV9653YUVVGAOptimalSet)/sizeof(OV9653YUVVGAOptimalSet[0]); i++)
		OV9653WriteByte(client, 
			OV9653YUVVGAOptimalSet[i].uRegister, 
			OV9653YUVVGAOptimalSet[i].uValue);
	printk("<1>Set %d registers value\n", i);
//	for(i=0; i< sizeof(OV9653YUVVGADefault)/sizeof(OV9653YUVVGADefault[0]); i++)
//		OV9653WriteByte(client, 
//			OV9653YUVVGADefault[i].uRegister, 
//			OV9653YUVVGADefault[i].uValue);
//	data->byWrite = OV9653_INIT;
//	i2c_smbus_write_byte(client, data->byWrite);
}


static void OV9653UpdateClient(struct i2c_client *client)
{
	OV9653DATA *data = client->data;

//	printk("<1>OV9653UpdateClient() be called\n");
	down(&data->UpdateLock);

	if ((jiffies - data->uLastUpdated > 5*HZ) ||
	    (jiffies < data->uLastUpdated) || !data->cValid) 
	{
#ifdef DEBUG
		printk("Starting OV9653 update\n");
#endif
		data->wRead = i2c_smbus_read_byte(client); 
		data->uLastUpdated = jiffies;
		data->cValid = 1;
	}
	up(&data->UpdateLock);
}

static int __init sm_OV9653_init(void)
{
	printk("<1>OV9653.o version %s (%s)\n", LM_VERSION, LM_DATE);
	return i2c_add_driver(&OV9653Driver);
}

static void __exit sm_OV9653_exit(void)
{
	i2c_del_driver(&OV9653Driver);
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
			OV9653WriteByte(pClient, reg.uRegister, reg.uValue);
			break;
		case PLSENSOR_READ_REGISTER:
			copy_from_user((void*)&reg , (void*)arg, sizeof(REGISTERINFO));
			reg.uValue=OV9653ReadByte(pClient, reg.uRegister);
			copy_to_user((void*)arg, (void*)&reg, sizeof(REGISTERINFO));
			break;
		case PLSENSOR_WRRD_REGISTER:
			copy_from_user((void*)&reg , (void*)arg, sizeof(REGISTERINFO));
			OV9653WriteByte(pClient, reg.uRegister, reg.uValue);
			reg.uValue=OV9653ReadByte(pClient, reg.uRegister);
			copy_to_user((void*)arg, (void*)&reg, sizeof(REGISTERINFO));
			break;
	}
	return 0;
}

static int OV9653SetFormat(PSENSOR pSensor)
{
	unsigned char		byHSTART, byHSTOP, byVSTART, byVSTOP, byTemp;
	int					bIsNotSXVGA=1;
	struct i2c_client	*pClient=pSensor->pI2CClient;
    int 				rc=0;
    
	if(pSensor->Format.nCropWidth == 1280 && pSensor->Format.nCropHeight == 960)
    {	//	SXVGA
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_SXVGA);
        pSensor->Format.uHSTART=292;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=15;		
        bIsNotSXVGA=0;
    }
    else if(pSensor->Format.nCropWidth == 1024 && pSensor->Format.nCropHeight == 768)
    {	//	XVGA
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_SXVGA);
        pSensor->Format.uHSTART=292;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=15;		
        bIsNotSXVGA=0;
    }
    else if (pSensor->Format.nCropWidth == 640 && pSensor->Format.nCropHeight == 480)
    {	//	VGA
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_VGA);
        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM3);
			byTemp |= (1<<2);
			OV9653WriteByte(pClient, OV9653REG_COM3, byTemp);
		}
        pSensor->Format.uHSTART=309;
        pSensor->Format.uVSTART=8;
        pSensor->Format.nFPS=30;		
    }
    else if (pSensor->Format.nCropWidth == 352 && pSensor->Format.nCropHeight == 288)
    {	//	CIF
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_CIF);
        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM3);
			byTemp |= (1<<2);
			OV9653WriteByte(pClient, OV9653REG_COM3, byTemp);
		}
        pSensor->Format.uHSTART=280;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=60;		
    }
    else if (pSensor->Format.nCropWidth == 320 && pSensor->Format.nCropHeight == 240)
    {	//	QVGA
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_QVGA);
        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM4);
			byTemp |= (1<<7);
			OV9653WriteByte(pClient, OV9653REG_COM4, byTemp);
		}
        pSensor->Format.uHSTART=142;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=60;		
    }
    else if (pSensor->Format.nCropWidth == 176 && pSensor->Format.nCropHeight == 144)
    {	//	QCIF
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_QCIF);
        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM4);
			byTemp |= (1<<7);
			OV9653WriteByte(pClient, OV9653REG_COM4, byTemp);
		}
        pSensor->Format.uHSTART=142;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=120;		
    }
    else if (pSensor->Format.nCropWidth == 160 && pSensor->Format.nCropHeight == 120)
    {	//	QQVGA
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_QVGA);
        byTemp=OV9653ReadByte(pClient, OV9653REG_COM1);
        byTemp |= (1<<5);
        OV9653WriteByte(pClient, OV9653REG_COM1, byTemp);

        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM4);
			byTemp |= (1<<7);
			OV9653WriteByte(pClient, OV9653REG_COM4, byTemp);
		}
        pSensor->Format.uHSTART=74;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=60;		
    }
    else if (pSensor->Format.nCropWidth == 88 && pSensor->Format.nCropHeight == 72)
    {	//	QQCIF
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_QCIF);
        byTemp=OV9653ReadByte(pClient, OV9653REG_COM1);
        byTemp |= (1<<5);
        OV9653WriteByte(pClient, OV9653REG_COM1, byTemp);

        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM4);
			byTemp |= (1<<7);
			OV9653WriteByte(pClient, OV9653REG_COM4, byTemp);
		}
        pSensor->Format.uHSTART=74;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=120;		
    }
    else	// unknown width and height...!!
    {	// default set as VGA 
		pSensor->Format.nCropWidth = 640;
		pSensor->Format.nCropHeight = 480;
        OV9653WriteByte(pClient, OV9653REG_COM7, COM7_VGA);
        if(pSensor->Format.nIsVarioPixel)
        {
			byTemp=OV9653ReadByte(pClient, OV9653REG_COM3);
			byTemp |= (1<<2);
			OV9653WriteByte(pClient, OV9653REG_COM3, byTemp);
		}
        pSensor->Format.uHSTART=280;
        pSensor->Format.uVSTART=6;
        pSensor->Format.nFPS=30;		
	}
	
	pSensor->Format.uHSTOP	=	pSensor->Format.uHSTART+
		(pSensor->Format.nCropWidth << bIsNotSXVGA);
	byHSTART= pSensor->Format.uHSTART>>3;
	byHSTOP = pSensor->Format.uHSTOP>>3;
	printk("<1>HSTART = 0x%02X (%u), HSTOP = 0x%02X (%u)\n", 
		byHSTART, byHSTART, byHSTOP, byHSTOP);
    OV9653WriteByte(pClient, OV9653REG_HSTART, byHSTART);
    OV9653WriteByte(pClient, OV9653REG_HSTOP, byHSTOP);
    byTemp=OV9653ReadByte(pClient, OV9653REG_HREF);
    byTemp = (byTemp & 0xC0) | (pSensor->Format.uHSTART & 0x07) | 
		((pSensor->Format.uHSTOP & 0x07)<<3);
    OV9653WriteByte(pClient, OV9653REG_HREF, byTemp);
	printk("<1>HREF = 0x%02X (%u)\n", byTemp, byTemp);
    
	pSensor->Format.uVSTOP	=	pSensor->Format.uVSTART+pSensor->Format.nCropHeight;
	byVSTART= pSensor->Format.uVSTART>>2;
	byVSTOP	= pSensor->Format.uVSTOP>>2;
    OV9653WriteByte(pClient, OV9653REG_VSTART, byVSTART);
    OV9653WriteByte(pClient, OV9653REG_VSTOP, byVSTOP);
    byTemp=OV9653ReadByte(pClient, OV9653REG_VREF);
    byTemp = (byTemp & 0xF0) | (pSensor->Format.uVSTART & 0x03) | 
		((pSensor->Format.uVSTOP & 0x03)<<2);
    OV9653WriteByte(pClient, OV9653REG_VREF, byTemp);
	pSensor->Format.nInterlaced=0;
	
	return 0;
}

static int OV9653GetFormat(PSENSOR pSensor)
{
//	unsigned short wData;
	return 0;	

}

static int OV9653SetSync(PSENSOR pSensor)
{
	int	value=0;

	if(pSensor->SyncInfo.byHRefSelect==DONT_CARE)
	{
		pSensor->SyncInfo.byITUBT656=0;
		pSensor->SyncInfo.byHRefSelect=1;
		value=OV9653ReadByte(pSensor->pI2CClient, OV9653REG_COM1);
		value &= 0xBF;	//	set external itu656
		OV9653WriteByte(pSensor->pI2CClient, OV9653REG_COM1, value);
	}
	else
	{
    	if (pSensor->SyncInfo.byITUBT656)
    	{
			value=OV9653ReadByte(pSensor->pI2CClient, OV9653REG_COM1);
			value |= 0x40;	//	set embedded itu656
			OV9653WriteByte(pSensor->pI2CClient, OV9653REG_COM1, value);
			pSensor->SyncInfo.byHRefSelect=0;
		}
		else
		{
			value=OV9653ReadByte(pSensor->pI2CClient, OV9653REG_COM1);
			value &= 0xBF;	//	set external itu656
			OV9653WriteByte(pSensor->pI2CClient, OV9653REG_COM1, value);
			pSensor->SyncInfo.byHRefSelect=1;
		}
	}

	value=OV9653ReadByte(pSensor->pI2CClient, OV9653REG_COM10);

	if (pSensor->SyncInfo.byHRefSelect)
	{
		value &= 0xBF;	//	set HREF
		value |= 0x03;	//	set both hsync and vsync to NEG
		pSensor->SyncInfo.byHSyncState = 0;
		pSensor->SyncInfo.byVSyncState = 0;
	}
	else
	{
		value |= 0x40;	//	set HSYNC
		value &= 0xFC;	//	set both hsync and vsync to POS
		pSensor->SyncInfo.byHSyncState = 1;
		pSensor->SyncInfo.byVSyncState = 1;
	}
	OV9653WriteByte(pSensor->pI2CClient, OV9653REG_COM10, value);
	return 0;
}

static int OV9653GetSync(PSENSOR pSensor)
{
	return 0;	
}

static void OV9653Restart(PSENSOR pSensor)
{
	HWResetSensor(pSensor->pI2CClient);
}

//
//===========================================================================

MODULE_AUTHOR("Chen Chao Yi<joe-chen@prolific.com.tw>");
MODULE_DESCRIPTION("OV9653 driver");
EXPORT_SYMBOL(g_pPLSensor);
module_init(sm_OV9653_init);
module_exit(sm_OV9653_exit);


