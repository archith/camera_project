/******************************************************************************
 *
 * Name:	skhwt.h
 * Project:	Genesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1.1.1.1 $
 * Date:	$Date: 2006/03/13 10:29:31 $
 * Purpose:	Defines for the hardware timer functions
 *
 ******************************************************************************/

/******************************************************************************
 *
 *	(C)Copyright 1989-1998 SysKonnect,
 *	a business unit of Schneider & Koch & Co. Datensysteme GmbH.
 *	All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF SYSKONNECT
 *	The copyright notice above does not evidence any
 *	actual or intended publication of such source code.
 *
 *	This Module contains Proprietary Information of SysKonnect
 *	and should be treated as Confidential.
 *
 *	The information in this file is provided for the exclusive use of
 *	the licensees of SysKonnect.
 *	Such users have the right to use, modify, and incorporate this code
 *	into products for purposes authorized by the license agreement
 *	provided they include this notice and the associated copyright notice
 *	with any such product.
 *	The information in this file is provided "AS IS" without warranty.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * History:
 *
 *	$Log: skgehwt.h,v $
 *	Revision 1.1.1.1  2006/03/13 10:29:31  jedy
 *	new 1029 kernel
 *	
 *	Revision 1.1.1.1  2006/02/15 03:45:42  jedy
 *	kernel
 *	
 *	Revision 1.1.1.1  2005/01/31 04:25:35  john
 *	
 *	1. Linux kernel 2.4.19
 *	2. porting on A320D
 *	
 *	Revision 1.1.1.1  2005/01/27 09:09:45  john
 *	
 *	2. porting on A320D
 *	
 *	Revision 1.1.1.1  2003/07/18 06:44:51  paulong
 *	Imported using TkCVS
 *	
 *	Revision 1.1.1.1  2003/07/17 12:33:53  paulong
 *	Imported using TkCVS
 *	
 *	Revision 1.1.1.1  2003/07/17 02:36:21  paulong
 *	armlinux with PCI/SD/IDE/MAC 20030717
 *	
 *	Revision 1.4  1998/08/19 09:50:58  gklug
 *	fix: remove struct keyword from c-code (see CCC) add typedefs
 *	
 *	Revision 1.3  1998/08/14 07:09:29  gklug
 *	fix: chg pAc -> pAC
 *	
 *	Revision 1.2  1998/08/07 12:54:21  gklug
 *	fix: first compiled version
 *	
 *	Revision 1.1  1998/08/07 09:32:58  gklug
 *	first version
 *	
 *	
 *	
 *	
 *
 ******************************************************************************/

/*
 * SKGEHWT.H	contains all defines and types for the timer functions
 */

#ifndef	_SKGEHWT_H_
#define _SKGEHWT_H_

/*
 * SK Hardware Timer
 * - needed wherever the HWT module is used
 * - use in Adapters context name pAC->Hwt
 */
typedef	struct s_Hwt {
	SK_U32		TStart ;	/* HWT start */
	SK_U32		TStop ;		/* HWT stop */
	int		TActive ;	/* HWT: flag : active/inactive */
} SK_HWT;

extern void SkHwtInit(SK_AC *pAC, SK_IOC Ioc);
extern void SkHwtStart(SK_AC *pAC, SK_IOC Ioc, SK_U32 Time);
extern void SkHwtStop(SK_AC *pAC, SK_IOC Ioc);
extern SK_U32 SkHwtRead(SK_AC *pAC,SK_IOC Ioc);
extern void SkHwtIsr(SK_AC *pAC, SK_IOC Ioc);
#endif	/* _SKGEHWT_H_ */
