/******************************************************************************
 *
 * Name:	version.h
 * Project:	GEnesis, PCI Gigabit Ethernet Adapter
 * Version:	$Revision: 1.1.1.1 $
 * Date:	$Date: 2006/03/13 10:29:31 $
 * Purpose:	SK specific Error log support
 *
 ******************************************************************************/

/******************************************************************************
 *
 *	(C)Copyright 1998,1999 SysKonnect,
 *	a business unit of Schneider & Koch & Co. Datensysteme GmbH.
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	The information in this file is provided "AS IS" without warranty.
 *
 ******************************************************************************/

/******************************************************************************
 *
 * History:
 *	$Log: skversion.h,v $
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
 *	Revision 1.1.1.1  2003/07/18 06:44:52  paulong
 *	Imported using TkCVS
 *	
 *	Revision 1.1.1.1  2003/07/17 12:33:54  paulong
 *	Imported using TkCVS
 *	
 *	Revision 1.1.1.1  2003/07/17 02:36:21  paulong
 *	armlinux with PCI/SD/IDE/MAC 20030717
 *	
 *	Revision 1.1  2001/03/06 09:25:00  mlindner
 *	first version
 *	
 *	
 *
 ******************************************************************************/
 
 
static const char SysKonnectFileId[] = "@(#)" __FILE__ " (C) SysKonnect GmbH.";
static const char SysKonnectBuildNumber[] =
	"@(#)SK-BUILD: 4.06 PL: 01"; 

#define BOOT_STRING	"sk98lin: Network Device Driver v4.06\n" \
			"Copyright (C) 2000-2001 SysKonnect GmbH."

#define VER_STRING	"4.06"


