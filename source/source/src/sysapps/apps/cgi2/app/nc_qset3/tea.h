/*
 * Copyright (C) 2005 SerComm Corporation. 
 * All Rights Reserved. 
 *
 * SerComm Corporation reserves the right to make changes to this document without notice. 
 * SerComm Corporation makes no warranty, representation or guarantee regarding the suitability 
 * of its products for any particular purpose. SerComm Corporation assumes no liability arising 
 * out of the application or use of any product or circuit. SerComm Corporation specifically 
 * disclaims any and all liability, including without limitation consequential or incidental 
 * damages; neither does it convey any license under its patent rights, nor the rights of others.
 */


#ifndef __TEA_H__
#define __TEA_H__

int TeaLenAlign64(int len);

/*
 * The buffer size must be the multiple of 64-bits.
 *	flag
 *		0		decode
 *		1		encode
 *	return
 *		0		success
 *		1		input buffer is not 64-bits align
 *		-1		errors
 */
int TeaTranslate(char *i_buf, int i_len, int flag, const unsigned long tea_key[4]);

#endif

