/* Linux driver for Philips webcam
   Decompression frontend.
   (C) 1999-2001 Nemosoft Unv. (webcam@smcc.demon.nl)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
/*
   This is where the decompression routines register and unregister 
   themselves. It also has a decompressor wrapper function.
*/

// topro header
#include "tp_def.h"
//#include "gcc.h"
#include <asm/types.h>
#include "qtable.h"
#include "pwc.h"
#include "pwc-uncompress.h"
#include <stdio.h>
#include "tp_api.h"

extern unsigned char topro_read_reg(struct usb_device *udev, unsigned char index);
extern int topro_write_reg(struct usb_device *udev, unsigned char index, unsigned char data);

unsigned char jpg_hd[] = {
	0xFF,0xD8,0xFF,0xC4,0x00,0x1D,0x00,0x00,0x02,0x03,0x01,0x01,0x01,0x01,0x01,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
	0x09,0xFF,0xC4,0x00,0x95,0x10,0x00,0x02,0x01,0x02,0x04,0x04,0x03,0x04,0x07,0x05,
	0x04,0x06,0x01,0x00,0x00,0x57,0x01,0x02,0x03,0x00,0x11,0x04,0x12,0x21,0x31,0x13,
	0x41,0x51,0x61,0x05,0x22,0x32,0x14,0x71,0x81,0x91,0x15,0x23,0x42,0x52,0x62,0xA1,
	0xB1,0x06,0x33,0x72,0xC1,0xD1,0x24,0x43,0x53,0x82,0x16,0x34,0x92,0xA2,0xE1,0xF1,
	0xF0,0x07,0x08,0x17,0x18,0x25,0x26,0x27,0x28,0x35,0x36,0x37,0x38,0x44,0x45,0x46,
	0x47,0x48,0x54,0x55,0x56,0x57,0x58,0x63,0x64,0x65,0x66,0x67,0x68,0x73,0x74,0x75,
	0x76,0x77,0x78,0x83,0x84,0x85,0x86,0x87,0x88,0x93,0x94,0x95,0x96,0x97,0x98,0xA3,
	0xA4,0xA5,0xA6,0xA7,0xA8,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xC2,0xC3,0xC4,0xC5, 
	0xC6,0xC7,0xC8,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,
	0xE8,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xFF,0xC4,0x00,0x1D,0x01,0x01,0x01,0x01, 
	0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x02,
	0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xC4,0x00,0x95,0x11,0x00,0x02,0x01,0x02,
	0x04,0x04,0x03,0x04,0x07,0x05,0x04,0x06,0x01,0x00,0x00,0x57,0x00,0x01,0x11,0x02,
	0x21,0x03,0x12,0x31,0x41,0x13,0x22,0x51,0x61,0x04,0x32,0x71,0x05,0x14,0x23,0x42,
	0x33,0x52,0x81,0x91,0xA1,0xB1,0xF0,0x06,0x15,0xC1,0xD1,0xE1,0x24,0x43,0x62,0xF1,
	0x16,0x25,0x34,0x53,0x72,0x82,0x92,0x07,0x08,0x17,0x18,0x26,0x27,0x28,0x35,0x36,
	0x37,0x38,0x44,0x45,0x46,0x47,0x48,0x54,0x55,0x56,0x57,0x58,0x63,0x64,0x65,0x66,
	0x67,0x68,0x73,0x74,0x75,0x76,0x77,0x78,0x83,0x84,0x85,0x86,0x87,0x88,0x93,0x94,
	0x95,0x96,0x97,0x98,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xB2,0xB3,0xB4,0xB5,0xB6,
	0xB7,0xB8,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,
	0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xFF,0xDB,
	0x00,0x43,0x00,0x10,0x0B,0x0C,0x0E,0x0C,0x0A,0x10,0x0E,0x0D,0x0E,0x12,0x11,0x10,
	0x13,0x18,0x28,0x1A,0x18,0x16,0x16,0x18,0x31,0x23,0x25,0x1D,0x28,0x3A,0x33,0x3D,
	0x3C,0x39,0x33,0x38,0x37,0x40,0x48,0x5C,0x4E,0x40,0x44,0x57,0x45,0x37,0x38,0x50,
	0x6D,0x51,0x57,0x5F,0x62,0x67,0x68,0x67,0x3E,0x4D,0x71,0x79,0x70,0x64,0x78,0x5C,
	0x65,0x67,0x63,0xFF,0xDB,0x00,0x43,0x01,0x11,0x12,0x12,0x18,0x15,0x18,0x2F,0x1A,
	0x1A,0x2F,0x63,0x42,0x38,0x42,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
	0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,
	0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,

  0x63,0x63,0x63,0x63,0x63,0x63,0x63,0x63,0xFF,0xC0,0x00,0x11,0x08,0x01,0xE0,0x02,//930406
	0x80,0x03,0x01,0x21,0x00,0x02,0x11,0x01,0x03,0x11,0x01,0xFF,0xDA,0x00,0x0C,0x03,

  0x01,0x00,0x02,0x11,0x03,0x11,0x00,0x3F,0x00
};

/* This contains a list of all registered decompressors */
static LIST_HEAD(pwc_decompressor_list);

/* Should the pwc_decompress structure ever change, we increase the
   version number so that we don't get nasty surprises, or can
   dynamicly adjust our structure.
 */
const int pwc_decompressor_version = PWC_MAJOR;
extern int HwMotion_Ythrld;
extern unsigned int packet_buffer_tmp;

/* Add decompressor to list, ignoring duplicates */
void pwc_register_decompressor(struct pwc_decompressor *pwcd)
{
	if (pwc_find_decompressor(pwcd->type) == NULL) {
		Trace(TRACE_PWCX, "Adding decompressor for model %d.\n", pwcd->type);
		list_add_tail(&pwcd->pwcd_list, &pwc_decompressor_list);
	}
}

/* Remove decompressor from list */
void pwc_unregister_decompressor(int type)
{
	struct pwc_decompressor *find;
	
	find = pwc_find_decompressor(type);
	if (find != NULL) {
		Trace(TRACE_PWCX, "Removing decompressor for model %d.\n", type);
		list_del(&find->pwcd_list);
	}
}

/* Find decompressor in list */
struct pwc_decompressor *pwc_find_decompressor(int type)
{
	struct list_head *tmp;
	struct pwc_decompressor *pwcd;

	list_for_each(tmp, &pwc_decompressor_list) {
		pwcd  = list_entry(tmp, struct pwc_decompressor, pwcd_list);
		if (pwcd->type == type)
			return pwcd;
	}
	return NULL;
}

void JpegUpdateDHqt(int select)
{
  int i;
  unsigned char *jpeghd;
  jpeghd = jpg_hd;
  for(i=0;i<64;i++){
    *(jpeghd + 371 + i) = InitQuantizationTable[select][i];
    *(jpeghd + 440 + i) = InitQuantizationTable[select][64+i];
  }
//  printk("Qtableindex %d\n", select);  
}  

int pwc_decompress(struct pwc_device *pdev)
{
	struct pwc_frame_buf *fbuf;
	int i,n, line, col, stride;
	void *yuv, *image, *dst;
	u16 *src;
	u16 *dsty, *dstu, *dstv;
  unsigned char QTindex;
  unsigned char *pBuffer;
  unsigned char *end1=0;
  int HwMotionPos = 0;
	if (pdev == NULL)
		return -EFAULT;
#if defined(__KERNEL__) && defined(PWC_MAGIC)
	if (pdev->magic != PWC_MAGIC) {
		Err("pwc_decompress(): magic failed.\n");
		return -EFAULT;
	}
#endif

	fbuf = pdev->read_frame;
	if (fbuf == NULL)
		return -EFAULT;
	image = pdev->image_ptr[pdev->fill_image];
	if (!image)
		return -EFAULT;

#if PWC_DEBUG
	/* This is a quickie */
	if (pdev->vpalette == VIDEO_PALETTE_RAW) {
		memcpy(image, fbuf->data, pdev->frame_size);
		return 0;
	}
#endif

	if ( pdev->type == 800 ) {
		/* if using TOPRO camera, copy the data to image buffer
		   and don't decompress the data. Besides, we add the
		   jpeg header for TOPRO camera in this. */
     //Update Qtable
     pBuffer = fbuf->data;
     end1 = pBuffer + fbuf->filled;

//     if(!(*(end1-2) == 0xff && *(end-1) == 0xd9))
//       printk("jpe end %x %x\n",*(end1-2),*(end1-1));
     
     if(*pBuffer == 0xff && *(pBuffer + 1) == 0xd8){

       QTindex =  *(pBuffer + 6);
       QTindex &= 0xf;
       QTindex = min((UCHAR)15, QTindex);

       if(pdev->QTableIndex != QTindex){
        pdev->QTableIndex = QTindex;     
        JpegUpdateDHqt(pdev->QTableIndex);

        //; printk("QTableIndex = %d\n",pdev->QTableIndex);
       }
     }

     if(HwMotion_Ythrld){
        HwMotionPos = *(pBuffer+2) << 8 | *(pBuffer+3);
        if(HwMotionPos){
          for(i=0 ; i<16 ;i++){
            if (i == 0)
              printk("\nHw Motion");             
            if (i % 4 == 0)
              printk("\n%d:",i/4);              
            if ((HwMotionPos >> (15-i)) & 0x01)
              printk("1 ");
            else
              printk("0 ");            
          }
          printk("\n");  
        }  
     }
     
//     printk("FrameCount %d\n",pdev->vframe_count);
  //   printk("Header[] %x %x %x %x %x %x %x %x\n", *(pBuffer), *(pBuffer+1), *(pBuffer+2),
  //            *(pBuffer+3), *(pBuffer+4), *(pBuffer+5), *(pBuffer+6), *(pBuffer+7));
    
     memcpy(image, jpg_hd, sizeof(jpg_hd));

//		printk("packet_buffer_tmp = %d\n", packet_buffer_tmp);

		memcpy(((char*)image)+sizeof(jpg_hd), fbuf->data + TP6830_HEADER_SIZE, fbuf->filled - TP6830_HEADER_SIZE);


    //printk("1 : %d\n",packet_buffer_tmp);
		packet_buffer_tmp = (fbuf->filled - TP6830_HEADER_SIZE);
//    printk("1 : %d %d %d\n",sizeof(jpg_hd), fbuf->filled, packet_buffer_tmp);
		return 0;
	}


	yuv = fbuf->data + pdev->frame_header_size;  /* Skip header */
	if (pdev->vbandlength == 0) {
		/* Uncompressed mode. We copy the data into the output buffer,
		   using the viewport size (which may be larger than the image
		   size). Unfortunately we have to do a bit of byte stuffing
		   to get the desired output format/size.
		 */
		switch (pdev->vpalette) {
		case VIDEO_PALETTE_YUV420:
			/* Calculate byte offsets per line in image & view */
			n   = (pdev->image.x * 3) / 2;
			col = (pdev->view.x  * 3) / 2;
			/* Offset into image */
			dst = image + (pdev->view.x * pdev->offset.y + pdev->offset.x) * 3 / 2;
			for (line = 0; line < pdev->image.y; line++) {
				memcpy(dst, yuv, n);
				yuv += n;
				dst += col;
			}
			break;

		case VIDEO_PALETTE_YUV420P:
			/*
			 * We do some byte shuffling here to go from the
			 * native format to YUV420P.
			 */
			src = (u16 *)yuv;
			n = pdev->view.x * pdev->view.y;

			/* offset in Y plane */
			stride = pdev->view.x * pdev->offset.y + pdev->offset.x;
			dsty = (u16 *)(image + stride);

			/* offsets in U/V planes */
			stride = pdev->view.x * pdev->offset.y / 4 + pdev->offset.x / 2;
			dstu = (u16 *)(image + n +         stride);
			dstv = (u16 *)(image + n + n / 4 + stride);

			/* increment after each line */
			stride = (pdev->view.x - pdev->image.x) / 2; /* u16 is 2 bytes */

			for (line = 0; line < pdev->image.y; line++) {
				for (col = 0; col < pdev->image.x; col += 4) {
					*dsty++ = *src++;
					*dsty++ = *src++;
					if (line & 1)
						*dstv++ = *src++;


					else
						*dstu++ = *src++;
				}
				dsty += stride;
				if (line & 1)
					dstv += (stride >> 1);
				else
					dstu += (stride >> 1);
			}
			break;
		}
	}
	else { 
		/* Compressed; the decompressor routines will write the data 
		   in interlaced or planar format immediately.
		 */
		if (pdev->decompressor)


			pdev->decompressor->decompress(
				&pdev->image, &pdev->view, &pdev->offset,
				yuv, image, 
				pdev->vpalette == VIDEO_PALETTE_YUV420P ? 1 : 0,
				pdev->decompress_data, pdev->vbandlength);
		else
			return -ENXIO; /* No such device or address: missing decompressor */
	}
	return 0;
}
#ifdef CUSTOM_QTABLE
//===========================================================================
// UpdateQuantizationTable
//===========================================================================


void UpdateQuantizationTable(void)
{
    UCHAR A_AC[16] = {1,1,3,1,5,3,7,2,5,3,7,4,5,6,7,8};
    UCHAR B_AC[16] = {4,2,4,1,4,2,4,1,2,1,2,1,1,1,1,1};
    UCHAR A_DC[16] = {1,1,3,1,1,1,5,5,3,3,7,7,2,2,5,5};
    UCHAR B_DC[16] = {4,2,4,1,1,1,4,4,2,2,4,4,1,1,2,2};
    int i,j;

    for (i=0;i<17;i++)
        {
//        InitQuantizationTable[i][0]  = 0x00;
//        InitQuantizationTable[i][65] = 0x01;
        //-----------------------------------------------------------
        //      DC
        //-----------------------------------------------------------
        if (i <= 15)
            {
            InitQuantizationTable[i][0] =
                max((unsigned short)min((unsigned short)(InitQuantizationTable[3][0] * A_DC[i] / B_DC[i]),(unsigned short)255), (unsigned short)4);

            InitQuantizationTable[i][64] =
                max((unsigned short)min((unsigned short)(InitQuantizationTable[3][64] * A_DC[i] / B_DC[i]),(unsigned short)255),(unsigned short)4);
            }
        else
            {
            InitQuantizationTable[i][0] = 4;
            InitQuantizationTable[i][64] =  4;
            }
        //-----------------------------------------------------------
        //      AC
        //-----------------------------------------------------------
        for (j=1;j<=63;j++)
            {
            if (i <= 15)
                {
                InitQuantizationTable[i][j] =
                    max((unsigned short)min((unsigned short)(InitQuantizationTable[3][j] * A_AC[i] / B_AC[i]),(unsigned short)255),(unsigned short)4);
                }
            else
                {
                InitQuantizationTable[i][j] = 4;
                }
//        printk("%x %x %x\n", i, j, InitQuantizationTable[i][j]);
            }

        for (j=65;j<=127;j++)
            {
            if (i <= 15)
                {
                InitQuantizationTable[i][j] =
                    max((unsigned short)min((unsigned short)(InitQuantizationTable[3][j] * A_AC[i] / B_AC[i]),(unsigned short)255),(unsigned short)4);
                }
            else
                {
                InitQuantizationTable[i][j] = 4;
                }
//        printk("%x %x %x\n", i, j, InitQuantizationTable[i][j]);
            }
        }
//	WriteQTableToFile();
}


void UpdateQTable(struct usb_device *udev, UCHAR value)
{
	UCHAR QTr[64] = {
		0,	1,	8,	16,	9,	2,	3,	10,
		17,	24,	32,	25,	18,	11,	4,	5,
		12,	19,	26,	33,	40,	48,	41,	34,
		27,	20,	13,	6,	7,	14,	21,	28,
		35,	42,	49,	56,	57,	50,	43,	36,
		29,	22,	15,	23,	30,	37,	44,	51,
		58,	59,	52,	45,	38,	31,	39,	46,
		54,	60,	61,	54,	47,	55,	62,	63
	};

	UCHAR Y_QT_VGA[64] = {
    0x08 ,0x05 ,0x06 ,0x07 ,0x06 ,0x05 ,0x08 ,0x07
    ,0x06 ,0x07 ,0x09 ,0x08 ,0x08 ,0x09 ,0x0c ,0x14
    ,0x0d ,0x0c ,0x0b ,0x0b ,0x0c ,0x18 ,0x11 ,0x12
    ,0x0e ,0x14 ,0x1d ,0x19 ,0x1e ,0x1e ,0x1c ,0x19
    ,0x1c ,0x1b ,0x20 ,0x24 ,0x2e ,0x27 ,0x20 ,0x22
    ,0x2b ,0x22 ,0x1b ,0x1c ,0x28 ,0x36 ,0x28 ,0x2b
    ,0x2f ,0x31 ,0x33 ,0x34 ,0x33 ,0x1f ,0x26 ,0x38
    ,0x3c ,0x38 ,0x32 ,0x3c ,0x2e ,0x32 ,0x33 ,0x31

		};

	UCHAR C_QT[64] = // The Q-table of the Chroma
	{
    0x11 ,0x12 ,0x12 ,0x18 ,0x15 ,0x18 ,0x2f ,0x1a
    ,0x1a ,0x2f ,0x63 ,0x42 ,0x38 ,0x42 ,0x63 ,0x63
    ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63
    ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63
    ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63
    ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63
    ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63
    ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63 ,0x63
	};
  
    int i, j, status;
	  UCHAR UpdateYTable[64],UpdateCTable[64],UpdateQTable[129];
    UCHAR RegData, VideoTiming;
//  	UCHAR LumaScale=0,ChromaScale=0,TempData,AutoQ_Func;
  	UCHAR LumaScale=0,ChromaScale=0;

		for(i=0;i<64;i++){
			UpdateYTable[i] = Y_QT_VGA[QTr[i]];
			if(UpdateYTable[i] > 0x1f)
				LumaScale = 1;
			UpdateCTable[i] = C_QT[i];
			if(UpdateCTable[i] > 0x3f)
				ChromaScale = 1;
		}

	  if(LumaScale){
		  for(i=0;i<64;i++){
			  UpdateYTable[i] /= 2;
//			if(UpdateYTable[i] < 4)
//				UpdateYTable[i] = 4;
		  }
	  }
    	if(ChromaScale){
		  for(i=0;i<64;i++){
			  UpdateCTable[i] /= 2;
//			if(UpdateCTable[i] < 4)
//				UpdateCTable[i] = 4;
		  }
	  }
//	DBGOUT5(("LumaScale %d ChromaScale %d",LumaScale,ChromaScale));
	  for(i=0;i<64;i++){
		  UpdateQTable[2*i] = UpdateYTable[i];
		  UpdateQTable[2*i+1] = UpdateCTable[i];
//		DBGOUT5(("CTable[%d] %x",i, UpdateCTable[i]));
	  }
//Stop VideoTiming
		VideoTiming = topro_read_reg(udev, VIDEO_TIMING);
  	VideoTiming &= 0xfe;
	  topro_write_reg(udev, VIDEO_TIMING, VideoTiming);
//Stop IsoChronous
	  topro_write_reg(udev, ENDP_1_CTL, STOP);

    RegData = topro_read_reg(udev, QTABLE_CFG);
	  RegData |= 0x01;  //Enable QTable Update
	  RegData &= 0xfd;  //Use Default Qtable
	  RegData |= (LumaScale << 2);
	  RegData |= (ChromaScale << 3);
	  topro_write_reg(udev, QTABLE_CFG, RegData);
	  topro_write_reg(udev, FIFO_CFG, 0);   //Reset initial address of the Q-table

    status = topro_bulk_out(udev, 4, UpdateQTable, 128);


    //Use the Updated QTable
    RegData |= 0x02;
    //Disable Qtable Update
    RegData &= 0xfe;
	  topro_write_reg(udev, QTABLE_CFG, RegData);

//	for(i=0;i<17;i++){
		for(j=0;j<64;j++){
			if(LumaScale)
				InitQuantizationTable[3][j+0] = UpdateYTable[j]*2;
			else
				InitQuantizationTable[3][j+0] = UpdateYTable[j];

			if(ChromaScale)
				InitQuantizationTable[3][j+64] = UpdateCTable[j]*2;
			else
				InitQuantizationTable[3][j+64] = UpdateCTable[j];

//      printk("%x %x\n", InitQuantizationTable[3][j+1], InitQuantizationTable[3][j+66]);
//			DBGOUT5(("IniYTable[%d] %x IniCTable[%d] %x",j, UpdateYTable[j]*(LumaScale+1), j, UpdateCTable[j]*(ChromaScale+1)));

//			DBGOUT5(("IniYTable[%d] %x IniCTable[%d] %x",j, InitQuantizationTable[3][j+1], j, InitQuantizationTable[3][j+65]));
		}
//	}
	  UpdateQuantizationTable();
    
//	  JpegReadDqt(DC->QTableIndex);


	  //open VideoTiming
	  VideoTiming |= 0x01;

    topro_write_reg(udev, VIDEO_TIMING, VideoTiming);

  //Open Isochronous
	  topro_write_reg(udev, ENDP_1_CTL, WAKE_UP);

///		AutoQ_Func = topro_read_reg(udev, AUTOQ_FUNC);
///	  AutoQ_Func |= 0x80;
///	  topro_write_reg(udev, AUTOQ_FUNC, AutoQ_Func);
}
#endif


/* Make sure these functions are available for the decompressor plugin
   both when this code is compiled into the kernel or as as module.
 */

EXPORT_SYMBOL_NOVERS(pwc_decompressor_version);
EXPORT_SYMBOL(pwc_register_decompressor);
EXPORT_SYMBOL(pwc_unregister_decompressor);
