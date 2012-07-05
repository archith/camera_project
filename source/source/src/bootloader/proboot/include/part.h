#ifndef __PART_H__
#define __PART_H__

#define PART_ENTRIES	8
#define PART_NAMELEN	8

#define PART_OFFSET	512

/*
 * part_info - partition information kept in memory
 */

struct part_info
{
	char	name[PART_NAMELEN];
	int	offset;			// unit: byte
	int	size;			// unit: byte
	int	head;			// unit: block
	int	tail;			// unit: block

	/* related SSFDC information */
	struct smdisk_info *smdisk;

	/* related NAND information */
	int	sz_block;
	int	ppb;
};

/*
 * part_table - partition table kept in the CIS block
 */

#define P_MAGIC		0x26542929
#define B_MAGIC		0x55aa

struct part_entry
{
	char	name[PART_NAMELEN];
	int	offset;
	int	size;
};

struct part_table
{
	unsigned int	pflag;		// certification flag
	unsigned int	pboot;		// bootable partition
	unsigned int	reserved[2];

	struct part_entry entry[PART_ENTRIES];
};

/*
 * routines
 */

int part_add (char *name, unsigned int offset, unsigned int size);
int part_del (char *name);
int part_format (char *name);
int part_setboot (char *name);
int part_save (void);
int part_load (void);
void part_show (char *name);
struct part_info *part_find (char *name);
int part_read_ecc (struct part_info *part, int blk, char *data, char *rdnt);
int part_read_oob (struct part_info *part, int blk, char *rdnt);
int part_write_ecc (struct part_info *part, int blk, char *data, char *rdnt);
int part_write_oob (struct part_info *part, int blk, char *rdnt);
int part_erase (struct part_info *part, int blk);
void part_init (void);

#endif // __PART_H__
