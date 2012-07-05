#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <openssl/evp.h>

#define ENCRYPTION	1
#define DECRYPTION	0

#define NUM_ALGO	10

#define BUF_SIZE	32
struct cipher_st
{
    char * name;
    const EVP_CIPHER *(*cip_ctor)(void);
};


static struct cipher_st function_array[NUM_ALGO] =
{
  {"aes128-ecb", EVP_aes_128_ecb},
  {"aes128-cbc", EVP_aes_128_cbc},
  {"aes192-ecb", EVP_aes_192_ecb},
  {"aes192-cbc", EVP_aes_192_cbc},
  {"aes256-ecb", EVP_aes_256_ecb},
  {"aes256-cbc", EVP_aes_256_cbc},
  {"des-ecb", EVP_des_ecb},
  {"des-cbc", EVP_des_cbc},
  {"des3-ecb", EVP_des_ede3_ecb},
  {"des3-cbc", EVP_des_ede3_cbc}
};

/*
    16 byte aligned
    |<------------->|<--------------->|<----->|<--->|
         in		  out		 key     iv
*/

void list_algo()
{
    int  i;
    
    printf("Supported algoritms are:\n");
    for (i = 0; i < NUM_ALGO; i++)
        printf("%s\n", function_array[i].name);
    exit(0);
}

void print_usage()
{
    printf("Usage: enc [options]\n");
    printf(" options:\n");
    printf("-a algorithm	the cipher algorithm to use\n");
    printf("-e | -d		encryption(default) or decryption\n");
    printf("-l			list algorithm supported\n");
    exit(0);
}

void hexdump(unsigned char *str, int len)
{
    int i;
    for (i=0; i<len; i++) {
        printf("%02x ", str[i]);
        if (!((i+1)&0xf))
            printf("\n");
    }
}

int main(int argc, char *argv[])
{
    EVP_CIPHER_CTX ctx;
    unsigned char *memchunk;
    int i;
    int next_option;
    int enc = 1, algo_idx;
    char *algo = NULL;
    unsigned char *key, *iv, *in, *out;
    unsigned char outm[16];
    int outl, inl=BUF_SIZE;

    do {
        next_option = getopt(argc, argv, "a:edl");
        
        if (next_option == -1)
            break;
        switch (next_option) {
            case 'a':
                algo = optarg;
                break;
            case 'e':
                enc = 1;
              break;
            case 'd':
                enc = 0;
              break;
            case 'l':
                list_algo();
            case '?':
            default:
                print_usage();
        }
    } while (next_option != -1);
    
    if (algo == NULL)
        print_usage();
    for (i=0; i<NUM_ALGO; i++) {
        if (strncmp(algo, function_array[i].name,
            strlen(function_array[i].name)) == 0) {
            break;
        }
    }
    if (i == NUM_ALGO) {
        printf("unknown algorithm:%s\n", algo);
        list_algo();
    }
    
    algo_idx = i;
    memchunk = malloc((BUF_SIZE << 1) + 32 + 16 + 15);
    if (!memchunk) {
        printf("Can't allocate memory\n");
        return -1;
    }
    
    in = (unsigned char *) (((unsigned)memchunk + 15) & 0xfffffff0);
    out = in + BUF_SIZE;
    key = out + BUF_SIZE;
    iv = key + 32;
    
    for (i=0; i<32; i++)
        key[i] = i;
    for (i = 0; i < 16; i++)
        iv[i] = i<<1;
    for (i=0; i< BUF_SIZE; i++) 
        in[i] = i & 0xff;
    EVP_CIPHER_CTX_init(&ctx);
    if (!EVP_CipherInit_ex(&ctx, function_array[algo_idx].cip_ctor(), NULL, key, iv, enc))
            return -1;
    EVP_CIPHER_CTX_set_padding(&ctx, 0);
    if (!EVP_CipherUpdate(&ctx, out, &outl, in, inl)) {
        printf("Can't encrypt.\n");
        return -1;
    }
    if (!EVP_CipherFinal_ex(&ctx, outm, &outl))
        printf("Final failed\n");

    EVP_CIPHER_CTX_cleanup(&ctx);
    printf("key=\n");
    hexdump(key, 32);
    printf("iv=\n");
    hexdump(iv, 16);
    printf("in=\n");
    hexdump(in, BUF_SIZE);
    printf("out=\n");
    hexdump(out, BUF_SIZE);
    free(memchunk);
    return 0;
    
}
