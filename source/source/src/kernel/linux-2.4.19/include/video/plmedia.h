#ifndef __PL_MEDIA_H__
#define __PL_MEDIA_H__

#include <linux/ioctl.h>
#include <linux/TimeStamp.h>

//===========================================================================
//	for both
typedef struct _tagRegisterInfo
{
	unsigned int	uRegister;
	unsigned int	uValue;
}	REGISTERINFO;
typedef	REGISTERINFO *PREGISTERINFO;
//
//===========================================================================

//===========================================================================
//	for encoder
#define ENCODE_AUTO				-1
#define TIME_INC_AUTO			-1
#define INVALID_ID				-1

#define I_VOP                   0
#define P_VOP                   1

#define ENCTYPE_H263			1
#define ENCTYPE_MPEG4			2
#define ENCTYPE_JPEG			4
//	jpeg quality level
#define JPEG_QLEV_VERYLOW		0
#define JPEG_QLEV_LOW			1
#define JPEG_QLEV_NORMAL		2	// avaliable now
#define JPEG_QLEV_HIGH			3	// avaliable now
#define JPEG_QLEV_VERYHIGH		4
#define JPEG_QLEV_CUST_1        5
#define JPEG_QLEV_CUST_2        6
#define JPEG_QLEV_CUST_3        7

#define DEFAULT_QTABLE_NUM		5
#define CUSTOM_QTABLE_NUM		3



#define QUANT_METHOD_ONE    	1
#define QUANT_METHOD_TWO    	0

// constant quality policies
#define ERC_POLICY_VBR			((2<<1) | 0)
#define ERC_POLICY_ANNEXL   	((1<<1) | 0)
// constant rate policies
#define ERC_POLICY_TMN8      	((1<<1) | 1)
#define ERC_POLICY_PL        	((2<<1) | 1)

#define BT_CMD_BUFF				0
#define BT_MDE_BUFF				1
#define BT_ACDC_BUFF			2
#define BT_MV_BUFF				3
#define BT_COLO_BUFF			4
#define BT_BS_BUFF				5
#define BT_FRAME_BUFF			6

#define BT_NUMNER				7

#define PLENC_MAGIC     		'O'

#define PLENC_NODUMP			0x00
#define PLENC_DCDUMP_EN			0x01
#define PLENC_MVDUMP_EN			0x02
#define PLENC_SETDUMP_MASK		0x03

#define PLENC_RESYNC_DIS		0x00
#define PLENC_RESYNC_FNUM		0x01
#define PLENC_RESYNC_FLEN		0x02

#define GB_PROGRESSIVE			0x00
#define GB_DEINTERLACED			0x01
#define GB_ODDEVENFIELD			0x02
#define GB_EVENODDFIELD			0x04
#define GB_ODDFIELD				0x10
#define GB_EVENFIELD			0x20
#define GB_ONEFIELDFULL			0x40
#define GB_ODD_AND_EVEN			0x80	
#define GB_BOTHFIELD			(GB_DEINTERLACED | GB_ODDEVENFIELD | GB_EVENODDFIELD)


//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	request frames to store the decoding result
typedef struct _tagQBuffInfo
{
	int				nType;
	int				nNumber;
	int				nSize;
}	QBUFFINFO;
typedef QBUFFINFO *PQBUFFINFO;

#define PLENC_REQ_BUFFER	_IOWR(PLENC_MAGIC, 1, QBUFFINFO)
#define PLENC_GET_BUFFER    _IOWR(PLENC_MAGIC, 2, unsigned int*)
#define PLENC_REL_BUFFER	_IOW (PLENC_MAGIC, 3, int*)
//#define PLDEC_REL_FRAME_BUFF	_IOW(PLDEC_MAGIC, 8, unsigned int*)
//---------------------------------------------------------------------------
//	direct access register; using logical address
#define PLENC_WRITE_REGISTER	_IOW (PLENC_MAGIC,	4, PREGISTERINFO)
#define PLENC_READ_REGISTER		_IOR (PLENC_MAGIC,	5, PREGISTERINFO)
#define PLENC_WRRD_REGISTER		_IOWR(PLENC_MAGIC,	6, PREGISTERINFO)
//---------------------------------------------------------------------------
//	To start decode; carry one parameter to tell decoder which command set it
//	has to use
//---------------------------------------------------------------------------

typedef struct _tagStartEncode
{
	int				nCodingType;
	int				nTimeInc;
	union
	{
		int				nJPEGQLevel;
		unsigned int	uPredNextSize;
	};
	unsigned int	uYAddr, uCbAddr, uCrAddr, uEndAddr;
	unsigned int	uBSOffset, uLength;
	unsigned int	uDCDumpAddr, uMVDumpAddr;
	unsigned int	uDCDumpSize, uMVDumpSize;
	unsigned char	byDCMVDumpUsed;
}	STARTENCODE;
typedef STARTENCODE *PSTARTENCODE;

#define PLENC_ENCODE		_IOWR (PLENC_MAGIC, 7, PSTARTENCODE)

typedef struct _tagEncodeFormat
{
	int				nEncType;	//	either jpeg or mpeg 4
	int				nScale, nWidth, nHeight;
	int				nEncW, nEncH;
	int				nFPS, nBPS;
	int				nKeyFrameRate, nQuantType, nVTIResolution, nVTI;
	unsigned int	uResyncFlag:16, uResyncValue:16;
	unsigned char	byPolicy, bySyncInterval;
}	ENCODEFORMAT;
typedef ENCODEFORMAT *PENCODEFORMAT;


typedef struct __tagQTables 
{
	int				nJPEGQLevel;
	unsigned char	u8LumaQTable[64];
	unsigned char	u8ChromaQTable[64];
}	QTABLE;
typedef QTABLE *PQTABLE;


#define PLENC_SET_ENC_FMT		_IOW (PLENC_MAGIC, 8, PENCODEFORMAT)
#define PLENC_GET_ENC_FMT		_IOR (PLENC_MAGIC, 9, PENCODEFORMAT)
#define PLENC_DEBUG_TEST		_IO  (PLENC_MAGIC, 10)
#define PLENC_SET_JPEG_QTABLE   _IOW (PLENC_MAGIC, 12, PQTABLE)
#define PLENC_GET_JPEG_QTABLE   _IOR (PLENC_MAGIC, 13, PQTABLE)

typedef struct _tagRCParameters
{
	int				nBPS, nKeyFrameRate;
	unsigned char	byPolicy;
}	RCPARAM;
typedef RCPARAM *PRCPARAM;

#define PLENC_CHG_RATECTRL		_IOW (PLENC_MAGIC, 14, PRCPARAM)

//
//===========================================================================

//===========================================================================
//	for grabber
#define PLGRAB_MAGIC     'E'
//---------------------------------------------------------------------------
#define PLGRAB_GET_SENSORTYPE		_IOR (PLGRAB_MAGIC, 0, int*)
//---------------------------------------------------------------------------
//	request frames to store the decoding result
#define PLGRAB_REQ_IMGBUFFINFO		_IOW (PLGRAB_MAGIC, 1, int*)
#define PLGRAB_GET_IMGBUFFINFO		_IOR (PLGRAB_MAGIC, 2, PIMAGEBUFFERINFO)
#define PLGRAB_REL_IMGBUFFINFO		_IO  (PLGRAB_MAGIC, 3)
//---------------------------------------------------------------------------
//	direct access register; using logical address
#define PLGRAB_WRITE_REGISTER	_IOW (PLGRAB_MAGIC,	4, PREGISTERINFO)
#define PLGRAB_READ_REGISTER	_IOR (PLGRAB_MAGIC,	5, PREGISTERINFO)
#define PLGRAB_WRRD_REGISTER	_IOWR(PLGRAB_MAGIC,	6, PREGISTERINFO)
//---------------------------------------------------------------------------
//	To start decode; carry one parameter to tell decoder which command set it
//	has to use
//---------------------------------------------------------------------------

#define PLGRAB_START_GRAB		_IO  (PLGRAB_MAGIC, 7)
#define PLGRAB_SET_VALID		_IOW (PLGRAB_MAGIC, 8, int*)
#define PLGRAB_GET_READY		_IOR (PLGRAB_MAGIC, 9, PREADYINFO)
#define PLGRAB_STOP_GRAB		_IO  (PLGRAB_MAGIC, 10)

#define DONT_CARE				0xFF

typedef struct _tagFramePerSecondEX
{	// FPS = uDenominator / uNumerator
	unsigned int	uNumerator;
	unsigned int	uDenominator;
}	FPSEX;
typedef FPSEX *PFPSEX;

typedef struct _tagExpectedSource
{
	int				nSrcWidth, nSrcHeight;
	int				nSrcFPS, nPossibleScaler;
	FPSEX			FPSEx;
}	EXPECTEDSOURCE;
typedef EXPECTEDSOURCE	*PEXPECTEDSOURCE;

typedef struct _tagGrabFormat
{
	int				nSrcWidth, nSrcHeight, nFPS;
	int				nCropWidth, nCropHeight;
	int				nUseHRef, nInterlaced;
	int				nEvenFieldFirst;
	FPSEX			FPSEx;
}	GRABFORMAT;
typedef GRABFORMAT *PGRABFORMAT;

#define PLGRAB_QUERY_GRAB_FMT	_IOW (PLGRAB_MAGIC, 24, PEXPECTEDSOURCE)
#define PLGRAB_SET_GRAB_FMT		_IOW (PLGRAB_MAGIC, 11, PGRABFORMAT)
#define PLGRAB_GET_GRAB_FMT		_IOR (PLGRAB_MAGIC, 12, PGRABFORMAT)

#define PLGRAB_CACHE_ON			1
#define PLGRAB_CACHE_OFF		0

#define PLGRAB_GRAB_ON			_IOW (PLGRAB_MAGIC, 13, int*)
#define PLGRAB_GRAB_OFF			_IO  (PLGRAB_MAGIC, 14)

/*
typedef struct _tagTimeCode
{
	unsigned int	uFlags;
	unsigned int	uType;
	unsigned char	pbyUserBits[4];
	unsigned char	byFrames;
	unsigned char	bySeconds;
	unsigned char	byMinutes;
	unsigned char	byHours;
}	TIMECODE;
typedef TIMECODE *PTIMECODE;
*/

typedef struct _tagImageBufferInfo
{
	stamp_t			ts90KTimeStamp;
	unsigned int	uStartAddr, uEndAddr, uTotalLength;
	unsigned int	uCbAddr, uCrAddr;
	unsigned int	uSequenceNo;
	int				nAccessCount;
}	IMAGEBUFFERINFO;

typedef IMAGEBUFFERINFO *PIMAGEBUFFERINFO;

typedef struct _tagReadyInfo
{
	stamp_t			ts90KTimeStamp;
	unsigned int	uSequenceNo;
	int				nIndex;
}	READYINFO;
typedef READYINFO *PREADYINFO;

//---------------------------------------------------------------------------
//	The sensor type should be TYPE_TVDECODER for below
typedef struct _tagQueryVideoInput
{
	unsigned int	uInputCapabilities;
	unsigned int	uCVFmtCapabilities;
}	QUERYVIDEOINPUT;
typedef QUERYVIDEOINPUT *PQUERYVIDEOINPUT;

typedef struct _tagVideoInput
{
	unsigned int	uInput;
	unsigned int	uCVFmt;
}	VIDEOINPUT;
typedef VIDEOINPUT *PVIDEOINPUT;

typedef struct _tagVideoConnection
{
	int				nConnectNum;
	unsigned int	uInput, uVSTD, uCVFmt;
	int				nIsColour, nSrcFreq;
}	VIDEOCONNECTION;
typedef VIDEOCONNECTION *PVIDEOCONNECTION;

#define PLGRAB_QUARY_INPUT		_IOR (PLGRAB_MAGIC, 15, PQUERYVIDEOINPUT)
#define PLGRAB_SET_INPUT		_IOW (PLGRAB_MAGIC, 16, PVIDEOINPUT)
#define PLGRAB_GET_INPUT		_IOR (PLGRAB_MAGIC, 17, PVIDEOINPUT)
#define PLGRAB_QUARY_VSTD		_IOR (PLGRAB_MAGIC, 18, unsigned int*)
#define PLGRAB_SET_VSTD			_IOW (PLGRAB_MAGIC, 19, unsigned int*)
#define PLGRAB_GET_VSTD			_IOR (PLGRAB_MAGIC, 20, unsigned int*)
#define PLGRAB_GET_CONNECT_NR	_IOR (PLGRAB_MAGIC, 21, int*)
#define PLGRAB_GET_CONNECTIONS	_IOR (PLGRAB_MAGIC, 22, PVIDEOCONNECTION)
#define PLGRAB_IS_CONNECTED		_IO  (PLGRAB_MAGIC, 23)

//
//===========================================================================

//===========================================================================
//	for sensor that mount on PL1029

#define PLSENSOR_MAGIC			'P'

#define PLSENSOR_WRITE_REGISTER	_IOW (PLSENSOR_MAGIC,	1, PREGISTERINFO)
#define PLSENSOR_READ_REGISTER	_IOR (PLSENSOR_MAGIC,	2, PREGISTERINFO)
#define PLSENSOR_WRRD_REGISTER	_IOWR(PLSENSOR_MAGIC,	3, PREGISTERINFO)
//
//===========================================================================


//===========================================================================
//	for motion detection
#define PLMD_MAGIC				'Q'

#define PLMD_REQ_BUFFER			_IOWR(PLMD_MAGIC, 1, QBUFFINFO)
#define PLMD_GET_BUFFER			_IOWR(PLMD_MAGIC, 2, unsigned int*)
#define PLMD_REL_BUFFER			_IOW (PLMD_MAGIC, 3, int*)
//
//===========================================================================

#define VSTD_STD_NTSC_M			0	// NTSC M (USA, ...)
#define VSTD_STD_PAL			1	// PAL B, G, D, H, I (...)
#define VSTD_STD_SECAM			2	// SECAM B, D, G, K, K1 (...)
#define VSTD_STD_NTSC_44		3	// PAL/NTSC hybrid
#define VSTD_STD_PAL_M			4	// (Brazil)
#define VSTD_STD_PAL_CN			5	// (Argentina, Paraguay, Uruguay)
#define VSTD_STD_PAL_60			6	// PAL/NTSC hybrid
#define VSID_STD_AUTODETC		7

#define VSTD_CAP_NTSC_M			(1<<VSTD_STD_NTSC_M)
#define VSTD_CAP_PAL			(1<<VSTD_STD_PAL)
#define VSTD_CAP_SECAM			(1<<VSTD_STD_SECAM)
#define VSTD_CAP_NTSC_44		(1<<VSTD_STD_NTSC_44)
#define VSTD_CAP_PAL_M			(1<<VSTD_STD_PAL_M)
#define VSTD_CAP_PAL_CN			(1<<VSTD_STD_PAL_CN)
#define VSTD_CAP_PAL_60			(1<<VSTD_STD_PAL_60)
#define VSTD_CAP_AUTODETC		(1<<VSID_STD_AUTODETC)
//  Values for the 'colorstandard' field
#define VSTD_COLOR_STD_PAL		1
#define VSTD_COLOR_STD_NTSC		2
#define VSTD_COLOR_STD_SECAM	3

//  Values for the color subcarrier fields
#define VSTD_COLOR_SUBC_PAL		4433619		// PAL BGHI, NTSC-44
#define VSTD_COLOR_SUBC_PAL_M	3575612		// PAL M (Brazil)
#define VSTD_COLOR_SUBC_PAL_CN	3582056		// PAL CN
#define VSTD_COLOR_SUBC_NTSC	3579545		// NTSC M, NTSC-Japan
#define VSTD_COLOR_SUBC_SECAMB	4250000		// SECAM B - Y carrier
#define VSTD_COLOR_SUBC_SECAMR	4406250		// SECAM R - Y carrier

//  Flags for the 'transmission' field
#define VSTD_TRANSM_STD_B		(1<<1)
#define VSTD_TRANSM_STD_D		(1<<3)
#define VSTD_TRANSM_STD_G		(1<<6)
#define VSTD_TRANSM_STD_H		(1<<7)
#define VSTD_TRANSM_STD_I		(1<<8)
#define VSTD_TRANSM_STD_K		(1<<10)
#define VSTD_TRANSM_STD_K1		(1<<11)
#define VSTD_TRANSM_STD_L		(1<<12)
#define VSTD_TRANSM_STD_M		(1<<13)
#define VSTD_TRANSM_STD_N		(1<<14)
//	Input Capability
#define VIN_COMPOSITE			0	//	AV-video, Tunner input 	{(Y+U+V)}
#define VIN_SVIDEO				1	//	S-Video input	{Y, (U+V)}
#define VIN_COMPONENT			2	//	YPbPr input		{Y, U, V}
#define VIN_PROGRESSIVE			3	//	???
#define VIN_TVTUNER				4	//	TV tuner input

#define VIN_CAP_COMPOSITE		(1<<VIN_COMPOSITE)
#define VIN_CAP_SVIDEO			(1<<VIN_SVIDEO)
#define VIN_CAP_COMPONENT		(1<<VIN_COMPONENT)
#define VIN_CAP_PROGRESSIVE		(1<<VIN_PROGRESSIVE)
#define VIN_CAP_TVTUNER			(1<<VIN_TVTUNER)

#define VIN_CVFMT_480I			0
#define VIN_CVFMT_576I			1
#define VIN_CVFMT_480P			2
#define VIN_CVFMT_576P			3
#define VIN_CVFMT_AUTO			8

#define VIN_CAP_CVMFT_480I		(1<<VIN_CVFMT_480I)
#define VIN_CAP_CVMFT_576I		(1<<VIN_CVFMT_576I)
#define VIN_CAP_CVMFT_480P		(1<<VIN_CVFMT_480P)
#define VIN_CAP_CVMFT_576P		(1<<VIN_CVFMT_576P)
#define VIN_CAP_CVMFT_AUTO		(1<<VIN_CVFMT_AUTO)

#define TYPE_SENSOR				1
#define TYPE_TVDECODER			2

#endif	// __PL_MEDIA_H__
