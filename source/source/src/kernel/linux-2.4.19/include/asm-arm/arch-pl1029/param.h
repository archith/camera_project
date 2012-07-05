#ifndef __ASM_ARCH_PARAM_H__
#define __ASM_ARCH_PARAM_H__

#define HZ          128

#if defined(__KERNEL__)
#define hz_to_std(a)        ((a*HZ)/100)
#endif

#endif /* __ASM_ARCH_PARAM_H__ */
