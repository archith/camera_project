#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <signal.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#define ENCRYPTION	1
#define DECRYPTION	0

#define NUM_ALGO	10

#define BUF_SIZE	512
/*copy from bits/sem.h */
union semun
{
     int val;				/*<= value for SETVAL */
     struct semid_ds *buf;		/*<= buffer for IPC_STAT & IPC_SET */
     unsigned short int *array;		/* <= array for GETALL & SETALL */
     struct seminfo *__buf;		/* <= buffer for IPC_INFO */
};
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
    |<------------->|<--------------->|<----------------->|<----->|<--->|
         in		  comp_out	     comp_in         key     iv
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
    printf("-l			list algorithm supported\n");
    exit(0);
}

pthread_t enc_thr_id, dec_thr_id;
int semid;

/* global data */
unsigned char *memchunk;
unsigned char *key, *iv, *in, *comp_out, *comp_in;
unsigned char outm[16];
int case_cnt;
int has_new;
int quit_asserted = 0;

void sig_int_handler(int signo)
{
    quit_asserted = 1;
}

void *enc_thr(void *arg)
{
    EVP_CIPHER_CTX ctx_enc;
    int i;
    int algo_idx = *(int *)arg;
    struct sembuf  sops;
    int outl, inl = BUF_SIZE;
    int ret;
    
    EVP_CIPHER_CTX_init(&ctx_enc);
    
    srandom(time(NULL));
    for (;;) {
        sops.sem_num = 0;
        sops.sem_op = -1;
        sops.sem_flg = 0;
        semop(semid, &sops, 1);
        if (has_new)
            goto again;
        for (i=0; i<32; i++)
            key[i] = (unsigned char)random();
        for (i = 0; i < 16; i++)
            iv[i] = (unsigned char) random();
        for (i=0; i< BUF_SIZE; i++) 
            in[i] = (unsigned char) random();
        if (!EVP_CipherInit_ex(&ctx_enc, function_array[algo_idx].cip_ctor(), NULL, key, iv, 1)) {
            ret =-1;
            goto out;
        }
        EVP_CIPHER_CTX_set_padding(&ctx_enc, 0);
        if (!EVP_CipherUpdate(&ctx_enc, comp_out, &outl, in, inl)) {
            printf("Can't encrypt.\n");
            ret = -1;
            goto out;
        }
        if (!EVP_CipherFinal_ex(&ctx_enc, outm, &outl)) {
            printf("ENC Final failed\n");
            ret = -1;
            goto out;
        }
        has_new = 1;
        printf("ENC: case %d done\n", case_cnt);
again:
        sops.sem_num = 0;
        sops.sem_op = 1;
        sops.sem_flg = 0;
        semop(semid, &sops, 1);
        if (quit_asserted)
            break;
    }
    EVP_CIPHER_CTX_cleanup(&ctx_enc);
    return (void *) 0;
out:
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    semop(semid, &sops, 1);
    return (void *) ret;
}

void *dec_thr(void *arg)
{
    EVP_CIPHER_CTX ctx_dec;
    int i;
    int algo_idx = *(int *)arg;
    struct sembuf  sops;
    int outl, inl = BUF_SIZE, equal;
    int ret;
    char errstr[256];
    
    EVP_CIPHER_CTX_init(&ctx_dec);
    ERR_load_crypto_strings();
    
    for (;;) {
        sops.sem_num = 0;
        sops.sem_op = -1;
        sops.sem_flg = 0;
        semop(semid, &sops, 1);
        if (!has_new)
            goto again;
        if (!EVP_CipherInit_ex(&ctx_dec, function_array[algo_idx].cip_ctor(), NULL, key, iv, 0)) {
            ret = -1;
            goto out;
        }
        EVP_CIPHER_CTX_set_padding(&ctx_dec, 0);
        if (!EVP_CipherUpdate(&ctx_dec, comp_in, &outl, comp_out, inl)) {
            printf("Can't encrypt.\n");
            ret =-1;
            goto out;
        }
        if (!EVP_CipherFinal_ex(&ctx_dec, outm, &outl)) {
            printf("DEC Final failed\n");
            while ((ret = ERR_get_error())) {
                ERR_error_string(ret, errstr);
                printf("%s\n", errstr);
            }    
            
            ret =-1;
            goto out;
        }
        
        equal =1;
        for (i=0; i<BUF_SIZE; i++){
            if (comp_in[i] != in[i]) {
                equal = 0;
                break;
            }
        }
        printf("DEC: case %d %s\n", case_cnt, ((equal)?("OK"):("FAIL")));
        case_cnt++;
        has_new = 0;
again:
        sops.sem_num = 0;
        sops.sem_op = 1;
        sops.sem_flg = 0;
        semop(semid, &sops, 1);
        
        if (quit_asserted)
            break;
    }
    EVP_CIPHER_CTX_cleanup(&ctx_dec);
    return (void *) 0;
out:
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = 0;
    semop(semid, &sops, 1);
    EVP_CIPHER_CTX_cleanup(&ctx_dec);
    return (void *) ret;
}

int main(int argc, char *argv[])
{
    int i;
    int next_option;
    int algo_idx;
    char *algo = NULL;
    union semun arg;
    struct sigaction act;
    void *retptr;
    int ret;

    do {
        next_option = getopt(argc, argv, "a:edl");
        
        if (next_option == -1)
            break;
        switch (next_option) {
            case 'a':
                algo = optarg;
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
    memchunk = malloc((BUF_SIZE << 1) +BUF_SIZE+ 32 + 16 + 15);
    if (!memchunk) {
        printf("Can't allocate memory\n");
        return -1;
    }
    
    in = (unsigned char *) (((unsigned)memchunk + 15) & 0xfffffff0);
    comp_out = in + BUF_SIZE;
    comp_in = comp_out + BUF_SIZE;
    key = comp_in + BUF_SIZE;
    iv = key + 32;
    
    semid = semget(IPC_PRIVATE, 1, 0);
    if (semid < 0) {
        printf("Can't create semaphore[%d]:%s\n", errno, strerror(errno));
        return -1;
    }
    arg.val = 1;
    if (semctl(semid, 0, SETVAL, arg) < 0) {
        printf("Can't set semaphore[%d]:%s\n", errno, strerror(errno));
        return -1;
    }
    /* install signal handler */
    act.sa_handler = sig_int_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    if (sigaction(SIGINT, &act, NULL) < 0)
        return -1;
    case_cnt = 0;
    has_new = 0;
    pthread_create(&enc_thr_id, NULL, enc_thr, &algo_idx); /* encrypt thread */
    pthread_create(&dec_thr_id, NULL, dec_thr, &algo_idx); /* verify thread (decrypt)*/
    if ((ret = pthread_join(enc_thr_id, &retptr)) != 0)
        printf("join enc thread failed[%d]\n", ret);
    if ((ret = pthread_join(dec_thr_id, &retptr)) != 0)
        printf("join enc thread failed[%d]\n", ret);
    return 0;
}

