#ifndef _FORMAT_CHECK_H_
#define _FORMAT_CHECK_H_

#define NUMBER		1
#define	NUM_ALPHA	2
#define	GRAPHIC		3
#define	PRINTABLE	4
#define	SERVER_ADDRESS	5
#define	EMAIL_ADDRESS	6

/* Check validation of characters in a string.
 * The validation depends on "range".
 *      range:  NUMBER
 *              NUM_ALPHA
 *              GRAPHIC
 *              PRINTABLE
 *              SERVER_ADDRESS
 *              EMAIL_ADDRESS
 */
int CheckChar(char* str, int range);

#endif
