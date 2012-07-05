#ifndef __STRING_H__
#define __STRING_H__
#include <stdarg.h>
int memcmp (const void *_s1, const void *_s2, int n);
void *memcpy (void *_dst, const void *_src, int n);
void *memset (void *s, int c, int n);
void memdump (const char *mem, int len);

char *strcat (char *dst, const char *src);
int strcmp (const char *s1, const char *s2);
char *strcpy (char *to, const char *from);
char *strncpy (char *to, const char *from, int length);
int strlen (const char *s);
int strncmp (const char *s1, const char *s2, int n);

char *strchr (const char *s, int c);
char *strstr (const char *s, const char *find);
char *strpbrk(const char * cs,const char * ct);
char *strsep(char **s, const char *ct);
int strnlen(const char * s, int count);
int vsprintf(char *buf, const char *fmt, va_list args);
int sprintf(char * buf, const char *fmt, ...);
#endif // __STRING_H__
