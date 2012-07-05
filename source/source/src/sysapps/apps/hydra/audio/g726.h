#include "audio_common.h"


typedef struct PutBitContext {
    uint32_t bit_buf;
    int bit_left;
    uint8_t *buf, *buf_ptr, *buf_end;
} PutBitContext;

typedef struct Float11 {
        int sign;   /**< 1bit sign */
        int exp;    /**< 4bit exponent */
        int mant;   /**< 6bit mantissa */
} Float11;

const uint8_t ff_log2_tab[256]={
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};


typedef struct G726Tables {
        int  bits;            /**< bits per sample */
        int* quant;           /**< quantization table */
        int* iquant;          /**< inverse quantization table */
        int* W;               /**< special table #1 ;-) */
        int* F;               /**< special table #2 */
} G726Tables;

typedef struct G726Context {
         G726Tables* tbls;    /**< static tables needed for computation */

         Float11 sr[2];       /**< prev. reconstructed samples */
         Float11 dq[6];       /**< prev. difference */
         int a[2];            /**< second order predictor coeffs */
         int b[6];            /**< sixth order predictor coeffs */
         int pk[2];           /**< signs of prev. 2 sez + dq */

         int ap;              /**< scale factor control */
         int yu;              /**< fast scale factor */
         int yl;              /**< slow scale factor */
         int dms;             /**< short average magnitude of F[i] */
         int dml;             /**< long average magnitude of F[i] */
         int td;              /**< tone detect */

         int se;              /**< estimated signal for the next iteration */
         int sez;             /**< estimated second order prediction */
         int y;               /**< quantizer scaling factor for the next iteration */
} G726Context;

static int quant_tbl16[] =                       /**< 16kbit/s 2bits per sample */
           { 260, INT_MAX };
static int iquant_tbl16[] =
           { 116, 365, 365, 116 };
static int W_tbl16[] =
           { -22, 439, 439, -22 };
static int F_tbl16[] =
           { 0, 7, 7, 0 };

static int quant_tbl24[] =                       /**< 24kbit/s 3bits per sample */
           {  7, 217, 330, INT_MAX };
static int iquant_tbl24[] =
           { INT_MIN, 135, 273, 373, 373, 273, 135, INT_MIN };
static int W_tbl24[] =
           { -4,  30, 137, 582, 582, 137,  30, -4 };
static int F_tbl24[] =
           { 0, 1, 2, 7, 7, 2, 1, 0 };

static int quant_tbl32[] =                       /**< 32kbit/s 4bits per sample */
           { -125,  79, 177, 245, 299, 348, 399, INT_MAX };
static int iquant_tbl32[] =
           { INT_MIN,   4, 135, 213, 273, 323, 373, 425,
                 425, 373, 323, 273, 213, 135,   4, INT_MIN };
static int W_tbl32[] =
           { -12,  18,  41,  64, 112, 198, 355, 1122,
            1122, 355, 198, 112,  64,  41,  18, -12};
static int F_tbl32[] =
           { 0, 0, 0, 1, 1, 1, 3, 7, 7, 3, 1, 1, 1, 0, 0, 0 };

static int quant_tbl40[] =                      /**< 40kbit/s 5bits per sample */
           { -122, -16,  67, 138, 197, 249, 297, 338,
              377, 412, 444, 474, 501, 527, 552, INT_MAX };
static int iquant_tbl40[] =
           { INT_MIN, -66,  28, 104, 169, 224, 274, 318,
                 358, 395, 429, 459, 488, 514, 539, 566,
                 566, 539, 514, 488, 459, 429, 395, 358,
                 318, 274, 224, 169, 104,  28, -66, INT_MIN };
static int W_tbl40[] =
           {   14,  14,  24,  39,  40,  41,   58,  100,
              141, 179, 219, 280, 358, 440,  529,  696,
              696, 529, 440, 358, 280, 219,  179,  141,
              100,  58,  41,  40,  39,  24,   14,   14 };
static int F_tbl40[] =
           { 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 6,
             6, 6, 5, 4, 3, 2, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };

static G726Tables G726Tables_pool[] =
           {{ 2, quant_tbl16, iquant_tbl16, W_tbl16, F_tbl16 },
            { 3, quant_tbl24, iquant_tbl24, W_tbl24, F_tbl24 },
            { 4, quant_tbl32, iquant_tbl32, W_tbl32, F_tbl32 },
            { 5, quant_tbl40, iquant_tbl40, W_tbl40, F_tbl40 }};


typedef struct AVG726Context {
   G726Context c;
   int bits_left;
   int bit_buffer;
   int code_size;
} AVG726Context;
