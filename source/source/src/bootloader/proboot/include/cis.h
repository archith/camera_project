#ifndef __CIS_H__
#define __CIS_H__

#define CIS_BLK	0	// assuming always at first physical block

int cis_chk (void *data);
void cis_dup (void *data, void *rdnt);

int cis_get (char *data, char *rdnt);
int cis_put (char *data, char *rdnt);

#endif
