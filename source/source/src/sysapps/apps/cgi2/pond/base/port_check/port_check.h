#ifndef _PORT_CHECK_H_
#define _PORT_CHECK_H_

/* Function:	port_check
 * Purpose:	check if the port is already in use
 * Parameter:	port_number - the port number to check
 * Return:	0 - available
 *		-1 - not available
 */
int port_check(unsigned short port_number);

#endif
