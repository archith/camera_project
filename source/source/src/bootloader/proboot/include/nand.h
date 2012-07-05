#ifndef __NAND_H__
#define __NAND_H__

#define MAX_BLK_NUM	16384		// refer to nand_ids.h
#define MAX_ZONE_NUM	16

#define MAX_DATA_LEN	(2048 * 64)	// data size in a big block
#define MAX_RDNT_LEN	(64   * 64)	// oob size in a big block

struct nand_chip {
	int	sz_page;
	int	sz_rdnt;
	int	sz_block;
	int	sz_device;

	int	ppb;
	int	bpd;

	unsigned char id[4];
};

extern struct nand_chip *(*nand_info) (void);
extern int (*nand_read_ecc) (int page, int pc, void *data, void *rdnt);
extern int (*nand_read_oob) (int page, void *rdnt);
extern int (*nand_write_ecc) (int page, int pc, void *data, void *rdnt);
extern int (*nand_write_oob) (int page, void *rdnt);
extern int (*nand_erase) (int page);
extern unsigned char g_nandinit; // after init flag

int nand_init (void);

#endif // __NAND_H__
