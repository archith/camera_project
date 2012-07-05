#ifndef	_AVI_FORMAT_H_
#define _AVI_FORMAT_H_

#define	_USE_OLD_INDEX_

#define FCC_RIFF	"RIFF"
#define FCC_LIST	"LIST"
/* chunk and list name fourcc */
#define FCC_AVI		"AVI "
#define FCC_HDRL	"hdrl"
#define FCC_AVIH	"avih"
#define FCC_STRL	"strl"
#define FCC_STRH	"strh"
#define FCC_STRF	"strf"
#define FCC_STRD	"strd"
#define FCC_STRN	"strn"
#define FCC_DMLH	"dmlh"
#define FCC_MOVI	"movi"
#define FCC_REC		"rec "
#define	FCC_IDX1	"idx1"
#define FCC_INDX	"indx"
/* data chunk id format */
#define	FCC_VIDEO_ID	"00dc"
#define FCC_AUDIO_ID	"01wb"
#define FMT_INDEX	"%02dix"


/* General type define */
typedef unsigned char	BYTE;	// 1 byte
typedef	unsigned short	WORD;	// 2 bytes
typedef unsigned int	DWORD;	// 4 bytes
typedef long		LONG;	// 4 bytes

struct quadword{
	DWORD	dwLow;	// low bytes of 64 bits
	DWORD	dwHigh;	// high bytes of 64 bits
};
typedef struct quadword	QWORD;	// 8 bytes


#define FOURCC_LEN	4
typedef	struct{
	DWORD	code;	// four character code and null termination
}__attribute__((packed)) FOURCC;

/* chunk alignment */
#define	CHUNK_ALIGN	sizeof(WORD)
typedef struct{
	FOURCC	fccname;
	LONG	size;
}__attribute__((packed)) ChunkHead;

typedef struct{
	ChunkHead	head;
	FOURCC		fcctype;
}__attribute__((packed)) ListHead;

/* Avi main header flags */
#define AVIF_HASINDEX		0x00000010
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000
#define AVIF_TRUSTCKTYPE	0x00000800	//openDML

/* Headers for avi */
/* Main avi header in 'avih' chunk */
typedef struct{
	DWORD	dwMicroSecPerFrame;	// video frame display rate
	DWORD	dwMaxBytesPerSec;	// Max data rate
	DWORD	dwPaddingGranularity;	// data alignment; pad data to multiple of this value
	DWORD	dwFlags;		// see avi main header flags
	DWORD	dwTotalFrames;		// total number of video frames in the RIFF-AVI list
	DWORD	dwInitialFrames;	// 0; (could be ignored)
	DWORD	dwStreams;		// number of streams in the file
	DWORD	dwSuggestedBufferSize;	// max bytes of a data chunk
	DWORD	dwWidth;		// width in pixels of the video stream
	DWORD	dwHeight;		// height in pixels of the video stream
#ifdef NOT_RESERVED
	DWORD	dwScale;		// time scale = dwRate/dwScale
	DWORD	dwRate;			// time scale unit: samples per sec

	DWORD	dwStart;		// start time, usually 0
	DWORD	dwLength;		// file length (sec * time scale)
#else
	DWORD	dwReserved[4];		// 0
#endif
}__attribute__((packed)) MainAVIHeader;


/* 'strh' type */
#define	FCC_VIDS	"vids"	// video
#define	FCC_AUDS	"auds"	// audio
#define FCC_TXTS	"txts"	// subtitle
/* 'strh' flags */
#define AVISF_DISABLED		0x00000001	// stream should not be activated by default
#define AVISF_VIDEO_PALCHANGES	0x00010000	// video stream using palette which changes
/* Avi stream header in 'strh' chunk */
typedef struct{
	FOURCC	fccType;		// type of data in a stream
	FOURCC	fccHandler;		// fourcc of the codec
	DWORD	dwFlags;		// (see 'strh' flags)
#ifdef NOT_RESERVED
	WORD	wPriority;		// the priority of a stream type
	WORD	wLanguage;		// language tag
#else
	DWORD	dwReserved1;		// 0
#endif
	DWORD	dwInitialFrames;	// How far audio data is skewed ahead of video frames

	/* dwScale and dwRate should be mutually prime 
	 * dwRate/dwScale = samples/second or frames/second 
	 */
	DWORD	dwScale;		// time scale
	/* For audio streams, this rate corresponds to the time needed to play nBlockAlign bytes of audio */
	DWORD	dwRate;			// 
	DWORD	dwStart;		// start time, usually 0
	DWORD	dwLength;		// stream length(sec * time scale)
	DWORD	dwSuggestedBufferSize;	// max bytes per frame of a stream
	DWORD	dwQuality;		// (ignored)
	DWORD	dwSampleSize;		// sample size; if 0, each chunk contains only one sample
					// for video, 0 if it varies;
					// for audio, same as nBlockAlign in WAVEFORAMTEX
	struct{
		short left;
		short top;
		short right;
		short bottom;
	}rcFrame;			// destination rectangle for this stream
}__attribute__((packed)) AVIStreamHeader;

/* Video format in 'strf' chunk */
typedef struct{
	DWORD	biSize;			// structure size
	LONG	biWidth;		// width in pixels
	LONG	biHeight;		// height in pixels
	WORD	biPlanes;		// 1
	WORD	biBitCount;		// number of bits per pixel: 24
	DWORD	biCompression;		// type of the compression
	DWORD	biSizeImage;		// max bytes per video frame
	LONG	biXPelsPerMeter;	// horizontal resolution: 0
	LONG	biYPelsPerMeter;	// vertical resolution: 0
	DWORD	biClrUsed;		// 0
	DWORD	biClrImportant;		// 0
	BYTE	CodecInfo[20];
}__attribute__((packed)) BitmapInfoHeader;

/* Audio format in 'strf' chunk */
typedef struct{
	WORD	wFormatTag;		// waveform-audio format type, refer to rfc2361
	WORD	nChannels;		// number of channels
	DWORD	nSamplesPerSec;		// sample rate
	DWORD	nAvgBytesPerSec;	// average data-transfer rate
	WORD	nBlockAlign;		// 1
	WORD	wBitsPerSample;		// bits per sample
	WORD	cbSize;			// 0
}__attribute__((packed)) WaveFormatEx;





/* OpenDML extened AVI header */
typedef struct{
	DWORD	dwTotalFrames;	// the real number of frames in the entire AVI file
}ODMLExtendedAVIHeader;



/* bIndexType flag */
#define	AVI_INDEX_OF_INDEXES	0x00	// each entry points to an index chunk
#define	AVI_INDEX_OF_CHUNKS	0x01	// each entry points to a chunk in the file
#define	AVI_INDEX_IS_DATA	0x80	// each entry points to real data
/* bIndexSubType flag for AVI_INDEX_OF_CHUNKS */
#define	AVI_INDEX_2FIELD	0x01	// each entry contains the offset of fields
/* OpenDML AVI index */
#define	AVI_SUPER_INDEX_NUM	1
#define	AVI_STD_INDEX_NUM	20
typedef struct{
	WORD	wLongsPerEntry;	// size of each entry in aIndex array
	BYTE	bIndexSubType;	// 
	BYTE	bIndexType;	// 
	DWORD	nEntriesInUse;	// number of entries in aIndex array that are used
	DWORD	dwChunkId;	// chunk id of what is indexed
	union{
		DWORD dwReserved[3];
		struct{
			QWORD	qwBaseOffset;
			DWORD	dwReserved;
		}sBaseOffset;
	}uReserved;
}__attribute__((packed)) AVIIndexHeader;
typedef struct{
	QWORD	qwOffset;	// absolute file offset
	DWORD	dwSize;		// size of index chunk at this offset
	DWORD	dwDuration;	// time span in stream time scale
}__attribute__((packed)) AVISuperIndexEntry;
typedef struct{
	DWORD	dwOffset;	// = absolute file offset - base offset
	DWORD	dwSize;		// bit 31 is set if this is NOT a key frame
}__attribute__((packed)) AVIStdIndexEntry;
typedef struct{
	AVIIndexHeader		IndexHdr;
	AVISuperIndexEntry	IndexEntry[AVI_SUPER_INDEX_NUM];
}__attribute__((packed)) AVISuperIndex;
typedef struct{
	AVIIndexHeader		IndexHdr;
	AVIStdIndexEntry	IndexEntry[AVI_STD_INDEX_NUM];
}__attribute__((packed)) AVIStdIndex;




/* AVI old index flags */
#define AVIIF_KEYFRAME	0x00000010
/* AVI old index entry */
typedef struct{
	FOURCC	dwChunkId;	// 'xxyy': xx - stream number
				// 	   yy - 'dc' for compressed video
				//		'wb' for audio
	DWORD	dwFlags;
	DWORD	dwOffset;	// the offset of the data chunk from the 
				// start of the 'movi' list (in bytes)
	DWORD	dwSize;		// the size of the data chunk (in bytes)
}__attribute__((packed)) AviOldIndexEntry;



typedef struct{
	ListHead		strl;
	ChunkHead		strh;
	AVIStreamHeader 	StreamHdr;
	ChunkHead		strf;
	BitmapInfoHeader	BitmapHdr;
#ifdef _USE_NEW_INDEX_
	ChunkHead		indx;
	AVISuperIndex		SuperIdx;
#endif
}__attribute__((packed)) AVIVideoStrl;
typedef struct{
	ListHead		strl;
	ChunkHead		strh;
	AVIStreamHeader 	StreamHdr;
	ChunkHead		strf;
	WaveFormatEx		WaveHdr;
#ifdef _USE_NEW_INDEX_
	ChunkHead		indx;
	AVISuperIndex		SuperIdx;
#endif
}__attribute__((packed)) AVIAudioStrl;

typedef struct{
	ListHead	hdrl;
	ChunkHead	avih;
	MainAVIHeader	MainHdr;
	AVIVideoStrl	VideoStrl;
	AVIAudioStrl*	AudioStrlPtr;
	size_t		size;
}__attribute__((packed)) AVIHdrlList;

typedef struct{
	ListHead	AVI;
	AVIHdrlList	HdrlList;
	ListHead	movi;
#ifdef _USE_OLD_INDEX_
	ChunkHead	idx1;
#endif
}__attribute__((packed)) AVIRiffHead;


#endif	// _AVI_FORMAT_H_
