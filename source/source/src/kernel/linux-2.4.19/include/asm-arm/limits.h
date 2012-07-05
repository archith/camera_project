#ifndef __ASM_PIPE_H
#define __ASM_PIPE_H

#ifndef PAGE_SIZE
#include <asm/page.h>
#endif

#ifdef CONFIG_PIPE_SIZE
#define PIPE_BUF    (PAGE_SIZE * (1 << CONFIG_PIPE_SIZE_ORD))
#else
#define PIPE_BUF	PAGE_SIZE
#endif

#endif

