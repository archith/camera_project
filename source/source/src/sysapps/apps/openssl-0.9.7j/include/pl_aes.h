#ifndef __PL_AES_H
#define __PL_AES_H


#ifndef __PL_ENGINE_DS
typedef void *PL_ENGINE;
#endif

/* define various types of ciphers-keylen-mode combination  */
#define DES_ECB		0x100
#define DES_CBC		0x110
#define TDES_ECB	0x200
#define TDES_CBC	0x210
#define AES128_ECB	0x300
#define AES128_CBC	0x310
#define AES192_ECB	0x400
#define AES192_CBC	0x410
#define AES256_ECB	0x500
#define AES256_CBC	0x510

/* function prototypes */
extern int pl_enc(PL_ENGINE e, unsigned type, int enc, unsigned char *key, unsigned char *iv, 
	unsigned char *in, unsigned char *out, unsigned int inl);
extern int pl_init_eng(PL_ENGINE *e);
extern int pl_shutdown_eng(PL_ENGINE e);
extern int pl_isopen(PL_ENGINE e);
extern unsigned char *pl_get_key(PL_ENGINE e);	
extern unsigned char *pl_get_iv(PL_ENGINE e);

#endif
