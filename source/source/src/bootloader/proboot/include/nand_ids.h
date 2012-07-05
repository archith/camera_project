#ifndef __NAND_IDS_H__
#define __NAND_IDS_H__

struct nand_ids {
	int id;
	int capacity;	// for PL1063
	int dt;		// for PL1061
	
	int sz_page;
	int sz_rdnt;
	int sz_block;
	int nr_block;
};

#define CAP512	((1 << 18) | (2 << 15) | (3 << 12))
#define CAP2K	((1 << 18) | (5 << 15) | (5 << 12))

#define DT_C3	((1 << 5) | (1 << 4) | (0 << 3))
#define DT_C4	((1 << 5) | (1 << 4) | (1 << 3))

static struct nand_ids ids[] =
{
	/* small blocks */
	{0x73, CAP512,	DT_C3,	512,	16,	16384,	1024},	// 16 MB
	{0x75, CAP512,	DT_C3,	512,    16,     16384,	2048},	// 32 MB
	{0x76, CAP512,	DT_C4,	512,    16,     16384,	4096},	// 64 MB
	{0x79, CAP512,	DT_C4,	512,    16,     16384,	8192},	// 128 MB
	{0x71, CAP512,	DT_C4,	512,    16,     16384,	16384},	// 256 MB

	/* big blocks */
	{0xf1, CAP2K,	0,	2048,	64,	131072,	1024},	// 128 MB
	{0xda, CAP2K,	0,	2048,	64,	131072,	2048},	// 256 MB
	{0xdc, CAP2K,	0,	2048,	64,	131072,	4096},	// 512 MB
	{0xd3, CAP2K,	0,	2048,	64,	131072,	8192},	// 1 GB
	{0xd5, CAP2K,	0,	2048,	64,	131072,	16384},	// 2 BG

	/* NULL terminated */
	{0},
};

#define NR_IDS (sizeof (ids) / sizeof (ids[0]))

#endif // __NAND_IDS_H__
