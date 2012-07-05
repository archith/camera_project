#ifndef __SMDISK__
#define __SMDISK__

#include <nand.h>

#define PHY_P2L_CIS	0

/*
 * smdisk_info - regards a sequence of physical blocks as a pseudo device
 */
enum mode_t {small, big};

struct smdisk_info
{
	short		p2l[MAX_BLK_NUM];
	short		candidate[MAX_ZONE_NUM];

	enum mode_t	mode;

	int		nr_phyblk;
	int		nr_logblk;

	/* related partition information */
	struct part_info *part;
};


/*
 * routines
 */

int smdisk_read  (char *name, int logblk, char *buf, int len);
int smdisk_write (char *name, int logblk, char *buf, int len);
struct smdisk_info *smdisk_register (struct part_info *part);
void smdisk_unregister (struct smdisk_info *smdisk);
void smdisk_showmap (struct smdisk_info *smdisk);
void smdisk_init (void);

#endif //__SMDISK
