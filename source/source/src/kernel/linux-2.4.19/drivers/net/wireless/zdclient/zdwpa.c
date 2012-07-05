#include "zd80211.h"

#define NONE			-1
#define WEP40			0
#define TKIP			1
#define AESCCMP			2
#define AESWRAP			3
#define WEP104			4
#define IEEE802_1X		0
#define ELEMENTID		0xdd
#define GROUPFLAG		0x02
#define REPLAYBITSSHIFT 2
#define REPLAYBITS		0x03

struct _IE {
	U8 Elementid;
	U8 length;
	U8 oui[4];
	U8 version;
	U8 multicast[4];
	U8 ucount;
	struct {
		U8 oui[4];
	} unicast[1]; // the rest is variable so need to overlay ieauth structure
}; 

struct _ieauth {
	U8 acount;
	struct {
		U8 oui[4];
	} auth[1];
};


BOOLEAN ParseWPA_IE(Element *pWpaIE, U8 length)
{
	U8 oui00[4] = { 0x00, 0x50, 0xf2, 0x00 };
	U8 oui01[4] = { 0x00, 0x50, 0xf2, 0x01 };
	U8 oui02[4] = { 0x00, 0x50, 0xf2, 0x02 };
	U8 oui03[4] = { 0x00, 0x50, 0xf2, 0x03 };
	U8 oui04[4] = { 0x00, 0x50, 0xf2, 0x04 };
	U8 oui05[4] = { 0x00, 0x50, 0xf2, 0x05 };
	struct _IE *IE = (struct _IE*) pWpaIE;
	int i = 0, j, m, n;
	struct _ieauth *ieauth;
	char *caps;

	wpaMulticast = TKIP;
	wpaUnicast[0] = TKIP;
	wpaUinCount = 1;
	wpaAuth[0] = IEEE802_1X;
	wpaAuthCount = 1;
	wpaUnicastAsGroup = 0;
	wpaReplayIndex = 2;

	// information element header makes sense
	if ( (IE->length+2 == length) && (IE->length >= 6)
  		&& (IE->Elementid == ELEMENTID)
		&& !memcmp(IE->oui, oui01, 4) && (IE->version == 1)) {
	    // update each variable if IE is long enough to contain the variable
		if (IE->length >= 10) {
			if (!memcmp(IE->multicast, oui01, 4))
				wpaMulticast = WEP40;
			else if (!memcmp(IE->multicast, oui02, 4))
				wpaMulticast = TKIP;
			else if (!memcmp(IE->multicast, oui03, 4))
				wpaMulticast = AESCCMP;
			else if (!memcmp(IE->multicast, oui04, 4))
				wpaMulticast = AESWRAP;
			else if (!memcmp(IE->multicast, oui05, 4))
				wpaMulticast = WEP104;
			else
				// any vendor checks here
				wpaMulticast = -1;
		}
		
		if (IE->length >= 12) {
			j = 0;
			for(i = 0; (i < IE->ucount) && (j < sizeof(wpaUnicast)/sizeof(int)); i++) {
				if(IE->length >= 12+i*4+4) {
					if (!memcmp(IE->unicast[i].oui, oui00, 4))
						wpaUnicast[j++] = NONE;
					else if (!memcmp(IE->unicast[i].oui, oui02, 4))
						wpaUnicast[j++] = TKIP;
					else if (!memcmp(IE->unicast[i].oui, oui03, 4))
						wpaUnicast[j++] = AESCCMP;
					else if (!memcmp(IE->unicast[i].oui, oui04, 4))
						wpaUnicast[j++] = AESWRAP;
					else
						// any vendor checks here
						;
				}
				else
					break;
			}
			wpaUinCount = j;			
		}
		
		m = i;
		if (IE->length >= 14+m*4) {
			// overlay ieauth structure into correct place
			ieauth = (struct _ieauth *)IE->unicast[m].oui;
			j = 0;
			for(i = 0; (i < ieauth->acount)  && (j < sizeof(wpaAuth)/sizeof(U8)); i++) {
				if(IE->length >= 14+4+(m+i)*4) {
					if (!memcmp(ieauth->auth[i].oui, oui00, 4))
						wpaAuth[j++] = IEEE802_1X;
					else
						// any vendor checks here
						;
				}
				else
					break;
			}
			if(j > 0)
				wpaAuthCount = j;
		}
		
		n = i;
		if(IE->length+2 >= 14+4+(m+n)*4) {
			caps = (char *)ieauth->auth[n].oui;
			wpaUnicastAsGroup = (*caps) & GROUPFLAG;
			wpaReplayIndex = 2 << ((*caps>>REPLAYBITSSHIFT) & REPLAYBITS);
		}
		return TRUE;
	}
	
	return FALSE;
}

char *cip[] = { "", " WEP40", " TKIP", " AES-CCMP", "AES-WRAP", "WEP104" };
char *cip1[] = { " NONE", " WEP40", " TKIP", " AES-CCMP", "AES-WRAP", "WEP104" };
char *aip[] = { "", " 802.1X" };

// Various IEs to try above with
U8 test1[] = {0xdd, 0x06, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00 };
U8 test2[] = {0xdd, 0x0a, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00, 
	0x00, 0x50, 0xf2, 0x01};
U8 test3[] = {0xdd, 0x10, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00, 
	0x00, 0x50, 0xf2, 0x01,
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x00};
U8 test4[] = {0xdd, 0x10, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00, 
	0x00, 0x50, 0xf2, 0x01,
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x02 };
U8 test5[] = {0xdd, 0x18, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00,
	0x00, 0x50, 0xf2, 0x01, 
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x02, 
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x00,
	0x06, 0x00 };
U8 test6[] = {0xdd, 0x1c, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00,
	0x00, 0x50, 0xf2, 0x02,
	0x02, 0x00, 0x00, 0x50, 0xf2, 0x02, 0x00, 0x50, 0xf2, 0x03, 
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x00,
	0x02, 0x00 };
// too small - ignored
U8 test7[] = {0xdd, 0x04, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00 };
// unicast count too high, 2nd unicast ignored and default auth
U8 test8[] = {0xdd, 0x16, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00,
	0x00, 0x50, 0xf2, 0x01, 
	0x02, 0x00, 0x00, 0x50, 0xf2, 0x02, 
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x00};
// unicast count past end of IE
U8 test9[] = {0xdd, 0x16, 0x00, 0x50, 0xf2, 0x01, 0x01, 0x00,
	0x00, 0x50, 0xf2, 0x01, 
	0x10, 0x00, 0x00, 0x50, 0xf2, 0x02, 
	0x01, 0x00, 0x00, 0x50, 0xf2, 0x00};


U8 *tests[] = { test1, test2, test3, test4, test5, test6, test7, test8, test9, NULL };
int testsize[] = {sizeof(test1), sizeof(test2), sizeof(test3),
				sizeof(test4), sizeof(test5), sizeof(test6), sizeof(test7),
				sizeof(test8), sizeof(test9), 0 };


