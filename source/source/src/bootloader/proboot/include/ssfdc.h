#ifndef __SSFDC_H__
#define __SSFDC_H__

#define LBPZ		1000		// number of logblk per zone
#define PBPZ		1024		// number of phyblk per zone
#define L2P_RATIO	LBPZ / PBPZ

#define P2L_CIS		(1000 + 1)
#define P2L_BAD		(1000 + 2)
#define P2L_UNUSED	(1000 + 4)
#define P2L_UNKNOWN	(1000 + 8)

/*
 * OOB definition of NAND flash with small/big blocks
 */

struct oob_512
{
	unsigned char	reserved[4];
	unsigned char	datastatus;
	unsigned char	blkstatus;
	unsigned char	blkaddr1[2];
	unsigned char	ecc2[3];
	unsigned char	blkaddr2[2];
	unsigned char	ecc1[3];
};

struct oob_2k
{
	unsigned char	blkstatus;
	unsigned char	reserved[7];
	unsigned char	datastatus;
	unsigned char	blkaddr1[2];
	unsigned char	ecc[3];
	unsigned char	blkaddr2[2];

	unsigned char	others[16 * 3];
};

int ssfdc_get_logblk (int isbig, char *rdnt);
void ssfdc_set_logblk (int isbig, unsigned char *rdnt, int lblk);

#endif // __SSFDC_H__
