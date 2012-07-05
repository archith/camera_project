#ifndef SC_MALLOC_H
#define SC_MALLOC_H
#include <cross.h>
void* sc_malloc(unsigned int size);
void dump(unsigned long addr, int size, char* name);
#endif
