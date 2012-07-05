typedef unsigned char BYTE;
typedef unsigned short WORD;   // 2 bytes
typedef unsigned int DWORD;  // 4 bytes
typedef long long QWORD;  // 8 bytes
typedef long LONG;   // 4 bytes
typedef struct ASF_HEADER
{
	BYTE objectID[16];  			   //   ASF_Header_Object
	DWORD objectLen;					//
	DWORD dummy;
	DWORD objectNum;					//   Number of header objects.
	BYTE reserved1; 				   //   Set the value to 1.
	BYTE reserved2; 				   //   Set the value to 2.
}  __attribute__((packed)) ASF_HEADER  ;
typedef struct S_ASF_FILE_PROPERTY
{
	BYTE objectID[16];  			  //   ASF_File_Properties_Object
	DWORD objectLen;				   //   This value must be larger than 104.
	DWORD dummy1;
	BYTE fileID[16];				  // @ Unique identifier for this file
	DWORD fileLen;  				   //   Invalid if Broadcast flag is set to 1.
	DWORD fileLen2; 					//   Invalid if Broadcast flag is set to 1.
	QWORD createDate;   			   // @ Maybe invalid if Broadcast flag is set to 1.
	DWORD dataPacketCount;  		   //   Must set to 1 to support Media Player v6.0
	DWORD dummy2;
	DWORD playDuration; 
	DWORD playDuration2;
	DWORD sendDuration;
	DWORD sendDuration2;
	QWORD preroll;  				   //   Amount of time to buffer data before play the file, in millisecond.
	DWORD broadcast : 1;	   //   It should be set to 1 if Broadcast is set to 1.
	DWORD seekable : 1; 	  //   Seekable. It should be set to 0.
	DWORD reserved : 30;	  //   Reserved and should be set to 0.
	DWORD minDataPacketLen; 		   // @ The Min. data packet size in bytes.
	DWORD maxDataPacketLen; 		   // @ The Max. data packet size in bytes.
	DWORD maxBitrate;   			   // @ The Max. instantaneous bitrate in bits per second for entire file.
} __attribute__((packed)) ASF_FILE_PROPERTY  ;
typedef struct S_ASF_VIDEO_MEDIA1
{
	DWORD encodedImageWidth;			// @ Encoded image width in pixels
	DWORD encodedImageHeigh;			// @ Encoded image heigh in pixels
	BYTE reserved1; 				   //   Reserved and should be set to 2.
	//WORD  formatDatalen;  			  //   Size of format data in bytes.
	BYTE formatDatalen; 			   //   Size of format data in bytes.
	BYTE dummy;
} __attribute__((packed)) ASF_VIDEO_MEDIA1 ;
typedef struct S_ASF_VIDEO_MEDIA2
{
	//------------------------
	DWORD formatDataSize;   			//   Size of format data in bytes.
	LONG imageWidth;				   // @ Encoded image width in pixels
	LONG imageHeigh;				   // @ Encoded image heigh in pixels
	WORD reserved2; 				   //   Reserved and should be set to 1
	WORD bitsPerPixel;  			   // @ Number of bits per pixel=24
	DWORD compressedID; 				//   Compression type
	DWORD imageSize;					// @ Size of image in bytes
	LONG hPixelPerMeter;			   //   Target horizontal resolution inpixels per meter
	LONG vPixelPerMeter;			   //   Target vertical resolution inpixels per meter
	DWORD colorsUsedCount;  			//   Color indexes in color table
	DWORD importantColorsCount; 		//   Required number of color indexes
	//-------------------------
} __attribute__((packed)) ASF_VIDEO_MEDIA2  ;
typedef struct S_ASF_MP4S_HEADER
{
	DWORD videoObjectStart; 			//   0x00010000 (Video object start code)
	DWORD VideoObjectLayerStart;		//   0x20010000 (Video object layer start code)
	BYTE video[10];
}__attribute__((packed)) ASF_MP4S_HEADER  ;
typedef struct S_ASF_VIDEO_MEDIA
{
	ASF_VIDEO_MEDIA1 m1;			  //size=11	
	ASF_VIDEO_MEDIA2 m2;			  //size=40
	ASF_MP4S_HEADER mp4Header;  		// @ Codec specific data type, size=18
} __attribute__((packed)) ASF_VIDEO_MEDIA  ;
typedef struct S_ASF_VIDEO_STREAM_PROPERTY
{
	BYTE objectID[16];  			   //   ASF_Stream_Properties_Object
	DWORD objectLen;					//   This value must be larger than 78.
	DWORD dummy;
	BYTE streamType[16];			   //   ASF_Video_Media
	BYTE errorCorrectionType[16];      //   Video stream: ASF_No_Error_Corrction
	QWORD timeOffset;   				//   Presentation time offset of the steam in 100-nanosecond, typically 0.
	DWORD specificDataLen;  			// @ Type specific data length in bytes
	DWORD errorCorrectionDataLen;   	//   Error correction data length in bytes
	WORD streamNum : 7; 	  //   Number of this stream (1~127, 0 is invalid)
	WORD reserved1 : 8; 	  //   Reserved and should be set to 0.
	WORD EncryptedContent : 1;  	 //   Set if stream is encrypted.
	DWORD reserved2;					//   Reserved and should be set to 0.
	//  ==> 78 bytes
	//-------------------------
	ASF_VIDEO_MEDIA videoMedia; 		// @ Type specific data, size=69
	//-------------------------
	// Error correction data			//   Error correction data
}  __attribute__((packed)) ASF_VIDEO_STREAM_PROPERTY  ;
typedef struct S_ASF_AUDIO_STREAM_PROPERTY1
{
	BYTE objectID[16];  			   //   ASF_Stream_Properties_Object
	//QWORD objectLen;  				  //   This value must be larger than 78.
	DWORD objectLen;					//   This value must be larger than 78.
	DWORD dummy;
	BYTE streamType[16];			   //   ASF_Video_Media
	BYTE errorCorrectionType[16];      //   Aideo stream: ASF_Audio_Spread
	QWORD timeOffset;   				//   Presentation time offset of the steam in 100-nanosecond, typically 0.
	DWORD specificDataLen;  			// @ Type specific data length in bytes
	DWORD errorCorrectionDataLen;   	//   Error correction data length in bytes
	WORD streamNum : 7; 	  //   Number of this stream (1~127, 0 is invalid)
	WORD reserved1 : 8; 	  //   Reserved and should be set to 0.
	WORD EncryptedContent : 1;  	 //   Set if stream is encrypted.
	DWORD reserved2;					//   Reserved and should be set to 0.
} __attribute__((packed))ASF_AUDIO_STREAM_PROPERTY1  ;
typedef struct S_ASF_AUDIO_STREAM_PROPERTY2
{
	//------------------------------------------------
	// ----  ASF_AUDIO_MEDIA audioMedia;		 // @ Type specific data
	//------------------------------------------------
	WORD codecID;   				   //   Codec ID (format tag), G726= 0x0045
	WORD channelNum;				   // @ Number of channels, 0x0001
	DWORD samplesPerChannel;			// @ Sampling rate in Hertz (cycles per second), 0x00001F40 (8000)
	DWORD bytesPerSecondWORD;   		// @ Average bytes per second, 0x00000FA0 (4000)
	//     G726_16000= 2000	7D0h
	//     G726_24000= 3000
	//     G726_32000= 4000    FA0h
	//     G726_40000= 5000
	//     G726_64000= ????
	WORD blockAlignment;			   //   Block alignment in bytes of the audio codec, 0x0001
	WORD bitsPerSample; 			   //   Bits per sample of mono data, 0x0004
	WORD codecSpecificDataLen;  	   //   Codec specific data size, set to 0
	//BYTE  codecSpecificData[];		  //   Codec specific data

	//------------------------------------------------
	//ASF_SPREAD_AUDIO audioSpread; 	  // @ Error correction data
	//------------------------------------------------
} __attribute__((packed))ASF_AUDIO_STREAM_PROPERTY2  ;
typedef struct S_ASF_AUDIO_STREAM_PROPERTY3
{
	BYTE span;  					   // @ Number of packets over which audio will be spread. 
	WORD virtualPacketLen;  		   // @ Set to the size of the largest audio payload found in the audio stream, 00A0(160)
	WORD virtualChunkLen;   		   // @ Set to the size of the largest audio payload found in the audio stream
	WORD slientDataLen; 			   //   Set to 1
	BYTE slientData[1]; 			  // @  Set to 0 
} __attribute__((packed)) ASF_AUDIO_STREAM_PROPERTY3  ;
typedef struct S_ASF_HEADER_EXTENSION
{
	BYTE objectID[16];  			   //   ASF_Header_Extension_Object
	//QWORD objectLen;  				  //   This value should be 46.
	DWORD objectLen;					//   This value should be 46.
	DWORD dummy;
	BYTE reserved1[16]; 			   //   ASF_Reserved_1
	WORD reserved2; 				   //   Reserved and should be set to 6.
	DWORD dataLen;  					//   0 (No additional data)
	//BYTE  data;   					  //   No additional data.
} __attribute__((packed)) ASF_HEADER_EXTENSION  ;
typedef struct ASF_DATA
{
	BYTE objectID[16];  			   //   ASF_DATA_Object
	DWORD objectLen;					//   Set the value to ffff if Broadcase flag is set to 1.
	DWORD dummy;
	BYTE fileID[16];				   // @ Same ad Field ID in File Property
	QWORD totalDataPackets; 			//   Invalid if Broadcast flag is set to 1.
	WORD reserved;  				   //   Set the value to 0x0101
} __attribute__((packed))ASF_DATA;
struct PayloadFirstPacketHeader
{
	BYTE PayloadFlags;
	BYTE StreamNum;		//81
	BYTE MediaObjNum;	//@ start from 0
	DWORD Offset;			//@ offset into media object
	BYTE RepDataLen;		//replicated data length =8
	DWORD FrameSize;		//@ frame total size
	DWORD PresentTime;	//@ presentation time
	WORD Len;			//@ payload length
} __attribute__((packed));
struct PayloadOtherPacketHeader
{
	BYTE PayloadFlags;
	BYTE StreamNum;	//PFrame=1
	BYTE MediaObjNum;	//@ start from 0
	DWORD Offset;		//@
	BYTE RepDataLen;	//replicated data length =0(PFrame)
	WORD Len;		//@ payload length		
} __attribute__((packed));
typedef struct ASF_ERROR_CORRECTION
{
	BYTE dataLength : 4;	   //    0002= 2-byte
	BYTE opaque : 1;	   //    0= No opaque data
	BYTE lengthType : 2;	   //    00= Size is defined in "dataLength" field
	BYTE present : 1;   	//    1= Error correction present

	BYTE type : 4;  	 //    0000= Data is uncorrected. (No Error Correction Object in the Header object)
	BYTE number : 4;	   //    0000= Set the value to 0 if data is uncorrected (type= 0000).
	BYTE Cycle; 					   //    0= Set the value to 0 if data is uncorrected (type= 0000).
} __attribute__((packed)) ASF_ERROR_CORRECTION;

typedef struct S_ASF_PAYLOAD_INFORMATION
{
	BYTE multiplePayload : 1;   	//!@ 1=
	BYTE sequenceType : 2;  	 //    00= No sequence
	BYTE paddingLengthType : 2; 	  //	10= Padding length type is WORD
	BYTE packetLengthType : 2;  	 //    00= No packet length
	BYTE errorCorrection : 1;   	//    0= No error correction

	BYTE replicatedType : 2;	   //01=byte
	BYTE offsetType : 2;	   //11=dword
	BYTE objectNumberType : 2;  	 //01=byte
	BYTE streamNumberType : 2;  	 //   01= Stream number is coded using a BYTE

	WORD paddingLength; 			   // @ Length of the padding at the end of a data packet
	DWORD sendTime; 					// @ Specified in milliseconds
	WORD duration;  				   // @ Specified in milliseconds
} __attribute__((packed)) ASF_PAYLOAD_INFORMATION  ;
typedef struct S_ASF_SAMPLE_INDEX_OBJECT
{
	BYTE ObjectID[16];
	QWORD ObjectSize;
	BYTE FileID[16];
	QWORD IndexEntryTimeInterval;
	DWORD MaximumPacketCount;
	DWORD IndexEntriesCount;
} __attribute__((packed)) ASF_SAMPLE_INDEX_OBJECT;
