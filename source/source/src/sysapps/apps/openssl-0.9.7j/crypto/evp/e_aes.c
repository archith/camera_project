/* ====================================================================
 * Copyright (c) 2001 The OpenSSL Project.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit. (http://www.openssl.org/)"
 *
 * 4. The names "OpenSSL Toolkit" and "OpenSSL Project" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    openssl-core@openssl.org.
 *
 * 5. Products derived from this software may not be called "OpenSSL"
 *    nor may "OpenSSL" appear in their names without prior written
 *    permission of the OpenSSL Project.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the OpenSSL Project
 *    for use in the OpenSSL Toolkit (http://www.openssl.org/)"
 *
 * THIS SOFTWARE IS PROVIDED BY THE OpenSSL PROJECT ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE OpenSSL PROJECT OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 */

#ifndef OPENSSL_NO_AES
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include <openssl/aes.h>
#include "evp_locl.h"

static int aes_init_key(EVP_CIPHER_CTX *ctx, const unsigned char *key,
					const unsigned char *iv, int enc);

typedef struct
	{
	AES_KEY ks;
	} EVP_AES_KEY;

#define data(ctx)	EVP_C_DATA(EVP_AES_KEY,ctx)

#ifdef PL_OPENSSL
static int pl_aes_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned type, unsigned char *out, 
	const unsigned char *in, unsigned int inl);

/* 
  * ecb mode *
  The original ecb mode implementation will enc/dec to the nearest multiple of block and disregard the rest
  we just assume the inl is multiple of 16 or error
*/
static int aes_128_ecb_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) 
{
	if (inl & 0xf)
	    return 0;
	return !pl_enc(ctx->plengine, AES128_ECB, ctx->encrypt, pl_get_key(ctx->plengine), pl_get_iv(ctx->plengine), 
		(unsigned char *)in, out, inl);
}
static int aes_192_ecb_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) 
{
	if (inl & 0xf)
	    return 0;
	return !pl_enc(ctx->plengine, AES192_ECB, ctx->encrypt, pl_get_key(ctx->plengine), pl_get_iv(ctx->plengine), 
		(unsigned char *)in, out, inl);
}
static int aes_256_ecb_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) 
{
	if (inl & 0xf)
	    return 0;
	return !pl_enc(ctx->plengine, AES256_ECB, ctx->encrypt, pl_get_key(ctx->plengine), pl_get_iv(ctx->plengine), 
		(unsigned char *)in, out, inl);
}

/* 
    * cbc mode *
    The cbc mode shall update the ctx->iv too 
    The cbc implementation of the original one will pad the in buffer with binary zero, and
      then run cbc. But this implies that the in and out buffer have extra spaces at the end.
*/
static int pl_aes_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned type, unsigned char *out, 
	const unsigned char *in, unsigned int inl)
{
	unsigned char ivtmp[16];
	int rc;

	if (inl & 0xf)
	    return 0;
	if ((in == out) && !ctx->encrypt)
	    memcpy(ivtmp, in+inl-16, 16);
	rc = pl_enc(ctx->plengine, type, ctx->encrypt, pl_get_key(ctx->plengine), pl_get_iv(ctx->plengine), 
		(unsigned char *)in, out, inl);
	if (!rc) {
	    if (ctx->encrypt)
	        memcpy(pl_get_iv(ctx->plengine), out+inl-16, 16);
	    else 
	        memcpy(pl_get_iv(ctx->plengine), ((in == out)? (ivtmp):(in+inl-16)), 16);
	    return 1;
	} else {
	    return 0;
	}
	
}

static int aes_128_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) 
{
        return pl_aes_cbc_cipher(ctx, AES128_CBC, out, in, inl);
}
static int aes_192_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) 
{
        return pl_aes_cbc_cipher(ctx, AES192_CBC, out, in, inl);
}
static int aes_256_cbc_cipher(EVP_CIPHER_CTX *ctx, unsigned char *out, const unsigned char *in, unsigned int inl) 
{
        return pl_aes_cbc_cipher(ctx, AES256_CBC, out, in, inl);
}

/* we don't touch the cfb ofb mode */
BLOCK_CIPHER_func_cfb(aes_128, AES, 128, EVP_AES_KEY, ks) 
BLOCK_CIPHER_func_ofb(aes_128, AES, 128, EVP_AES_KEY, ks) 
BLOCK_CIPHER_func_cfb(aes_192, AES, 128, EVP_AES_KEY, ks) 
BLOCK_CIPHER_func_ofb(aes_192, AES, 128, EVP_AES_KEY, ks) 
BLOCK_CIPHER_func_cfb(aes_256, AES, 128, EVP_AES_KEY, ks) 
BLOCK_CIPHER_func_ofb(aes_256, AES, 128, EVP_AES_KEY, ks) 

BLOCK_CIPHER_defs(aes_128, EVP_AES_KEY, NID_aes_128, 16, 16, 16, 
			  128, EVP_CIPH_FLAG_FIPS, aes_init_key, 
			  NULL, EVP_CIPHER_set_asn1_iv, 
			  EVP_CIPHER_get_asn1_iv, NULL)
BLOCK_CIPHER_defs(aes_192, EVP_AES_KEY, NID_aes_192, 16, 24, 16, 
			  128, EVP_CIPH_FLAG_FIPS, aes_init_key, 
			  NULL, EVP_CIPHER_set_asn1_iv, 
			  EVP_CIPHER_get_asn1_iv, NULL)
BLOCK_CIPHER_defs(aes_256, EVP_AES_KEY, NID_aes_256, 16, 32, 16, 
			  128, EVP_CIPH_FLAG_FIPS, aes_init_key, 
			  NULL, EVP_CIPHER_set_asn1_iv, 
			  EVP_CIPHER_get_asn1_iv, NULL)

#else
IMPLEMENT_BLOCK_CIPHER(aes_128, ks, AES, EVP_AES_KEY,
		       NID_aes_128, 16, 16, 16, 128,
		       EVP_CIPH_FLAG_FIPS, aes_init_key, NULL, 
		       EVP_CIPHER_set_asn1_iv,
		       EVP_CIPHER_get_asn1_iv,
		       NULL)
IMPLEMENT_BLOCK_CIPHER(aes_192, ks, AES, EVP_AES_KEY,
		       NID_aes_192, 16, 24, 16, 128,
		       EVP_CIPH_FLAG_FIPS, aes_init_key, NULL, 
		       EVP_CIPHER_set_asn1_iv,
		       EVP_CIPHER_get_asn1_iv,
		       NULL)
IMPLEMENT_BLOCK_CIPHER(aes_256, ks, AES, EVP_AES_KEY,
		       NID_aes_256, 16, 32, 16, 128,
		       EVP_CIPH_FLAG_FIPS, aes_init_key, NULL, 
		       EVP_CIPHER_set_asn1_iv,
		       EVP_CIPHER_get_asn1_iv,
		       NULL)
#endif /* ifdef PL_OPENSSL */

#define IMPLEMENT_AES_CFBR(ksize,cbits,flags)	IMPLEMENT_CFBR(aes,AES,EVP_AES_KEY,ks,ksize,cbits,16,flags)

IMPLEMENT_AES_CFBR(128,1,EVP_CIPH_FLAG_FIPS)
IMPLEMENT_AES_CFBR(192,1,EVP_CIPH_FLAG_FIPS)
IMPLEMENT_AES_CFBR(256,1,EVP_CIPH_FLAG_FIPS)

IMPLEMENT_AES_CFBR(128,8,EVP_CIPH_FLAG_FIPS)
IMPLEMENT_AES_CFBR(192,8,EVP_CIPH_FLAG_FIPS)
IMPLEMENT_AES_CFBR(256,8,EVP_CIPH_FLAG_FIPS)

static int aes_init_key(EVP_CIPHER_CTX *ctx, const unsigned char *key,
		   const unsigned char *iv, int enc)
	{
	int ret;

#ifdef PL_OPENSSL
	/* although we can use AES_KEY to get the original key, we still store the key in ctx */
	memset(pl_get_key(ctx->plengine), 0, /*FIXME*/ 32);
	memcpy(pl_get_key(ctx->plengine), key, ctx->key_len);
	memset(pl_get_iv(ctx->plengine), 0, /* FIXME */16);
	memcpy(pl_get_iv(ctx->plengine), iv, ctx->cipher->iv_len);
#endif
	if ((ctx->cipher->flags & EVP_CIPH_MODE) == EVP_CIPH_CFB_MODE
	    || (ctx->cipher->flags & EVP_CIPH_MODE) == EVP_CIPH_OFB_MODE
	    || enc) 
		ret=AES_set_encrypt_key(key, ctx->key_len * 8, ctx->cipher_data);
	else
		ret=AES_set_decrypt_key(key, ctx->key_len * 8, ctx->cipher_data);

	if(ret < 0)
		{
		EVPerr(EVP_F_AES_INIT_KEY,EVP_R_AES_KEY_SETUP_FAILED);
		return 0;
		}

	return 1;
	}

#endif
