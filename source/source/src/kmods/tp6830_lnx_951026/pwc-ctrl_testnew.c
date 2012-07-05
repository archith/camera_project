/* Driver for Philips webcam
   Functions that send various control messages to the webcam, including
   video modes.
   (C) 1999-2001 Nemosoft Unv. (webcam@smcc.demon.nl)

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

/*
   Changes
   2001/08/03  Alvarado   Added methods for changing white balance and
                          red/green gains
 */

/* Control functions for the cam; brightness, contrast, video mode, etc. */

/* =================================================================================
	03/03/31 ¥[¤JTOPRO's camera control command
   ================================================================================= */


#ifdef __KERNEL__
#include <asm/uaccess.h>
#endif
#include <asm/errno.h>
#include <linux/usb.h>

#include "pwc.h"
#include "pwc-ioctl.h"
#include "pwc-uncompress.h"
#include "osd1.h"
// topro header
#include "tp_def.h"
#include "pwc-ctrl-TI.h"

#include "tp_gamma_r.h"
#include "tp_gamma_g.h"
#include "tp_gamma_b.h"
#include "tp_gamma_y.h"
#include "tp_gamma2_r.h"  //950903
#include "tp_gamma2_g.h"  //950903
#include "tp_gamma2_b.h"  //950903

#include "tp_param.h"
#include "lnx.h"





