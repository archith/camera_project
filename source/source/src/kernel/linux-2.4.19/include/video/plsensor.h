#ifndef __PL_SENSOR_H__
#define __PL_SENSOR_H__

#include <linux/i2c.h>
#include <linux/i2c-proc.h>
#include <video/plmedia.h>

typedef struct _tagSensorSync
{
	int				nHSyncStart;
	int				nVSyncStart;
	int				nHSyncPhase;
	unsigned char	byITUBT656;
	unsigned char	byHRefSelect;
	unsigned char	byHSyncState;
	unsigned char	byVSyncState;
	unsigned char	byValidEnable;
	unsigned char	byValidState;
} 	SENSORSYNC;

typedef struct _tagSensorFormat
{
	int				nSrcWidth, nSrcHeight;
	int				nCropWidth, nCropHeight;
	int				nInterlaced, nFPS, nIsVarioPixel, nEvenFieldFirst;
	unsigned int	uHSTART, uHSTOP, uVSTART, uVSTOP;
	FPSEX			FPSEx;
} 	SENSORFORMAT;

typedef struct _tagVideoStandard
{
	unsigned char			szName[24];
	struct 
	{
		unsigned int		uNumerator;
		unsigned int		uDenominator;	// 
	} 	FrameRate;							// Frames, not fields
	unsigned int			uFrameLines;
	unsigned int			uFields;
	unsigned int			uColourStandard;
	union 
	{
		struct 
		{	// Fsc : Frequence of Subcarrier
			unsigned int	uColourSubcarrier; // Hz
		} 	PAL;
		struct 
		{	// Fsc : Frequence of Subcarrier
			unsigned int	uColourSubcarrier; //
		} 	NTSC;
		struct 
		{	// Fsc : Frequence of Subcarrier
			unsigned int	uf0b;	// Hz (blue)
			unsigned int	uf0r;	// Hz (red) 
		} 	SECAM;
	} 	ColourStandardData;
	unsigned int			uTransmission;
}	VIDEOSTANDARD;
	
typedef VIDEOSTANDARD *PVIDEOSTANDARD;

typedef struct _tagSensor
{
	struct list_head	OPSList;
    const char  		*szShortName;       // device name
    const char  		*szFullName;     	// device description name
    const char  		*szVersion;       	// grab_dev driver version
	struct i2c_client	*pI2CClient;
    void 				*PrivateData;
	int					nType;
	SENSORSYNC			SyncInfo;
	SENSORFORMAT		Format;
	unsigned int		uInputCapabilities;
	unsigned int		uCVFmtCapabilities;
	unsigned int 		uVSTDCapabilities;
	unsigned int		uInput;
	unsigned int		uCVFmt;
	unsigned int		uVSTD;
	VIDEOSTANDARD		VideoStandard;
	int					nInterlaced;
	//	interfaces	
	void (*Restart)(struct _tagSensor *pSensor);
	int (*QueryFormat)(struct _tagSensor* pSensor, int w, int h, int fps);
	int (*SetFormat)(struct _tagSensor *pSensor);
	int (*GetFormat)(struct _tagSensor *pSensor);
	int (*SetSync)(struct _tagSensor *pSensor);
	int (*GetSync)(struct _tagSensor *pSensor);
	int (*SetInput)(struct _tagSensor *pSensor);
	int (*GetInput)(struct _tagSensor *pSensor);
	int (*SetVSTD)(struct _tagSensor *pSensor);
	int (*GetVSTD)(struct _tagSensor *pSensor);
	int (*ShowStatus)(struct _tagSensor *pSensor);

//	unsigned int (*ReadRegister)(struct _tagSensor *pSensor, unsigned int reg);
//	int	(*WriteRegister)(struct _tagSensor *pSensor, unsigned int reg, unsigned int value);
} 	SENSOR;
typedef SENSOR *PSENSOR;

#endif // __PL_SENSOR_H__
