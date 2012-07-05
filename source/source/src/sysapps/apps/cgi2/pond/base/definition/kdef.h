#include <stdio.h>
#define IN
#define OUT
#define INOUT
#define EQ_ERR(a, b) if(a==b) {fprintf(stderr, "%s:LINE=%d, %d == %d\n", __FILE__, __LINE__, (int)a , (int)b);goto errout;}
#define NE_ERR(a, b) if(a!=b) {fprintf(stderr, "%s:LINE=%d, %d != %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
#define GT_ERR(a, b) if(a>b) {fprintf(stderr, "%s:LINE=%d, %d > %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
#define LS_ERR(a, b) if(a<b) {fprintf(stderr, "%s:LINE=%d, %d < %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
#define EXP_ERR(a) if(a) {fprintf(stderr, "%s:LINE=%d, expression true\n", __FILE__, __LINE__);goto errout;}
#define EXP_ERR_STR(a, format, arg...) if(a) {fprintf(stderr, "%s:LINE=%d, expression true "format"\n", __FILE__, __LINE__, ##arg);goto errout;}
#define EXP_ERR_ERRCODE(a, err) if(a) {fprintf(stderr, "%s:LINE=%d, expression true\n", __FILE__, __LINE__);errcode = err; goto errout;}
#define EXP_ERR_ERRCODE_STR(a, err,format, arg...) if(a) {fprintf(stderr, "%s:LINE=%d, expression true::"format"\n", __FILE__, __LINE__,##arg);errcode = err; goto errout;}
#define EQ_GOTO(a, b) if(a==b) {goto errout;}
#define ERROUT() {fprintf(stderr, "%s:LINE=%d\n", __FILE__, __LINE__);goto errout;}
#define ERROUT_ERRCODE(err) {fprintf(stderr, "%s:LINE=%d\n", __FILE__, __LINE__);errcode = err;goto errout;}
#define EQ_ERR_ERRCODE(a, b, err) if(a==b) {fprintf(stderr, "%s:LINE=%d, %d == %d\n", __FILE__, __LINE__, (int)a , (int)b);errcode = err; goto errout;}
#define NE_ERR_ERRCODE(a, b, err) if(a!=b) {fprintf(stderr, "%s:LINE=%d, %d != %d\n", __FILE__, __LINE__, (int)a , (int)b);errcode = err; goto errout;}
#define GT_ERR_ERRCODE(a, b, err) if(a>b) {fprintf(stderr, "%s:LINE=%d, %d > %d\n", __FILE__, __LINE__, (int)a , (int)b);errcode = err; goto errout;}
#define LS_ERR_ERRCODE(a, b, err) if(a<b) {fprintf(stderr, "%s:LINE=%d, %d < %d\n", __FILE__, __LINE__, (int)a , (int)b);errcode = err; goto errout;}
#define EP_ERR_ERRCODE(a, err) if(a) {fprintf(stderr, "%s:LINE=%d, expression true\n", __FILE__, __LINE__);errcode = err;goto errout;}
#define EQ_ERR_STR(a, b, format, arg...) if(a==b) {fprintf(stderr, "%s:LINE=%d, %d == %d  "format"\n", __FILE__, __LINE__, (int)a , (int)b, ##arg);goto errout;}
#define NE_ERR_STR(a, b, format, arg...) if(a!=b) {fprintf(stderr, "%s:LINE=%d, %d != %d  "format"\n", __FILE__, __LINE__, (int)a , (int)b, ##arg);goto errout;}
#define LS_ERR_STR(a, b, format, arg...) if(a<b) {fprintf(stderr, "%s:LINE=%d, %d == %d  "format"\n", __FILE__, __LINE__, (int)a , (int)b, ##arg);goto errout;}
#define EQ_ERR_ERRCODE_STR(a, b, err, format, arg...) if(a==b) {fprintf(stderr, "%s:LINE=%d, %d == %d  "format"\n", __FILE__, __LINE__, (int)a , (int)b, ##arg);errcode=err;goto errout;}
#define KMIN(a, b) a>b?b:a
#define KMAX(a, b) a>b?a:b
//#define MAX(a, b) a>b?a:b

