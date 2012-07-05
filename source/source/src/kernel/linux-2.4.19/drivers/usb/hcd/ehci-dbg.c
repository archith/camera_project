/*
 * Copyright (c) 2001 by David Brownell
 * 
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* this file is part of ehci-hcd.c */

#ifdef EHCI_VERBOSE_DEBUG
#define vdbg dbg   
#else
	static inline void vdbg (char *fmt, ...) { }
#endif

#ifdef	DEBUG

/* check the values in the HCSPARAMS register - host controller structural parameters */
/* see EHCI 0.95 Spec, Table 2-4 for each value */
static void dbg_hcs_params (struct ehci_hcd *ehci, char *label)
{
	u32	params = readl (&ehci->caps->hcs_params);

	dbg ("%s hcs_params 0x%x dbg=%d%s cc=%d pcc=%d%s%s ports=%d",
		label, params,
		HCS_DEBUG_PORT (params),
		HCS_INDICATOR (params) ? " ind" : "",
		HCS_N_CC (params),
		HCS_N_PCC (params),
	        HCS_PORTROUTED (params) ? "" : " ordered",
		HCS_PPC (params) ? "" : " !ppc",
		HCS_N_PORTS (params)
		);
	/* Port routing, per EHCI 0.95 Spec, Section 2.2.5 */
	if (HCS_PORTROUTED (params)) {
		int i;
		char buf [46], tmp [7], byte;

		buf[0] = 0;
		//Bruce;;for (i = 0; i < HCS_N_PORTS (params); i++) {
		//Bruce;;	byte = readb (&ehci->caps->portroute[(i>>1)]);
		//Bruce;;	sprintf(tmp, "%d ", 
		//Bruce;;		((i & 0x1) ? ((byte)&0xf) : ((byte>>4)&0xf)));
		//Bruce;;	strcat(buf, tmp);
		//Bruce;;}
		dbg ("%s: %s portroute %s", 
			ehci->hcd.bus_name, label,
			buf);
	}
}
#else

static inline void dbg_hcs_params (struct ehci_hcd *ehci, char *label) {}

#endif

#ifdef	DEBUG

/* check the values in the HCCPARAMS register - host controller capability parameters */
/* see EHCI 0.95 Spec, Table 2-5 for each value */
static void dbg_hcc_params (struct ehci_hcd *ehci, char *label)
{
	u32	params = readl (&ehci->caps->hcc_params);

	if (HCC_EXT_CAPS (params)) {
		// EHCI 0.96 ... could interpret these (legacy?)
		dbg ("%s extended capabilities at pci %d",
			label, HCC_EXT_CAPS (params));
	}
	if (HCC_ISOC_CACHE (params)) {
		dbg ("%s hcc_params 0x%04x caching frame %s%s%s",
		     label, params,
		     HCC_PGM_FRAMELISTLEN (params) ? "256/512/1024" : "1024",
		     HCC_CANPARK (params) ? " park" : "",
 		     HCC_64BIT_ADDR (params) ? " 64 bit addr" : "");
	} else {
		dbg ("%s hcc_params 0x%04x caching %d uframes %s%s%s",
		     label,
		     params,
		     HCC_ISOC_THRES (params),
		     HCC_PGM_FRAMELISTLEN (params) ? "256/512/1024" : "1024",
		     HCC_CANPARK (params) ? " park" : "",
 		     HCC_64BIT_ADDR (params) ? " 64 bit addr" : "");
	}
}
#else

static inline void dbg_hcc_params (struct ehci_hcd *ehci, char *label) {}

#endif

#ifdef	DEBUG

#if 0
static void dbg_qh (char *label, struct ehci_hcd *ehci, struct ehci_qh *qh)
{
	dbg ("%s %p info1 %x info2 %x hw_curr %x qtd_next %x", label,
		qh, qh->hw_info1, qh->hw_info2,
		qh->hw_current, qh->hw_qtd_next);
	dbg ("  alt+errs= %x, token= %x, page0= %x, page1= %x",
		qh->hw_alt_next, qh->hw_token,
		qh->hw_buf [0], qh->hw_buf [1]);
	if (qh->hw_buf [2]) {
		dbg ("  page2= %x, page3= %x, page4= %x",
			qh->hw_buf [2], qh->hw_buf [3],
			qh->hw_buf [4]);
	}
}
#endif

static const char *const fls_strings [] =
    { "1024", "512", "256", "??" };

#else
#if 0
static inline void dbg_qh (char *label, struct ehci_hcd *ehci, struct ehci_qh *qh) {}
#endif
#endif	/* DEBUG */

/* functions have the "wrong" filename when they're output... */

#define dbg_status(ehci, label, status) \
	dbg ("%s status 0x%x%s%s%s%s%s%s%s%s%s%s", \
		label, status, \
		(status & STS_ASS) ? " Async" : "", \
		(status & STS_PSS) ? " Periodic" : "", \
		(status & STS_RECL) ? " Recl" : "", \
		(status & STS_HALT) ? " Halt" : "", \
		(status & STS_IAA) ? " IAA" : "", \
		(status & STS_FATAL) ? " FATAL" : "", \
		(status & STS_FLR) ? " FLR" : "", \
		(status & STS_PCD) ? " PCD" : "", \
		(status & STS_ERR) ? " ERR" : "", \
		(status & STS_INT) ? " INT" : "" \
		)

#define dbg_cmd(ehci, label, command) \
	dbg ("%s %x cmd %s=%d ithresh=%d%s%s%s%s period=%s%s %s", \
		label, command, \
		(command & CMD_PARK) ? "park" : "(park)", \
		CMD_PARK_CNT (command), \
		(command >> 16) & 0x3f, \
		(command & CMD_LRESET) ? " LReset" : "", \
		(command & CMD_IAAD) ? " IAAD" : "", \
		(command & CMD_ASE) ? " Async" : "", \
		(command & CMD_PSE) ? " Periodic" : "", \
		fls_strings [(command >> 2) & 0x3], \
		(command & CMD_RESET) ? " Reset" : "", \
		(command & CMD_RUN) ? "RUN" : "HALT" \
		)

#define dbg_port(hcd, label, port, status) \
	dbg ("%s port %d status 0x%x%s%s speed=%d%s%s%s%s%s%s%s%s%s", \
		label, port, status, \
		(status & PORT_OWNER) ? " OWNER" : "", \
		(status & PORT_POWER) ? " POWER" : "", \
		(status >> 10) & 3, \
		(status & PORT_RESET) ? " RESET" : "", \
		(status & PORT_SUSPEND) ? " SUSPEND" : "", \
		(status & PORT_RESUME) ? " RESUME" : "", \
		(status & PORT_OCC) ? " OCC" : "", \
		(status & PORT_OC) ? " OC" : "", \
		(status & PORT_PEC) ? " PEC" : "", \
		(status & PORT_PE) ? " PE" : "", \
		(status & PORT_CSC) ? " CSC" : "", \
		(status & PORT_CONNECT) ? " CONNECT" : "" \
	    )





//=========================  Bruce::Start ============================================
#ifdef	CONFIG_USB_DEBUG 

static inline char * edstring(unsigned int ed_type,char *tmp) 
{
  switch (ed_type) {
  case PIPE_CONTROL:
        strcpy(tmp,"ctrl");
        break;
  case PIPE_BULK:
        strcpy(tmp, "bulk");
        break;
  case PIPE_INTERRUPT:
        strcpy(tmp,"intr");
        break;
  default:     
        strcpy(tmp,"isoc");
        break;
  };
  return tmp;
}

#define pipestring(pipe, tmp_str) edstring(usb_pipetype(pipe),tmp_str)

/* debug| print the main components of an URB
 * small: 0) header + data packets 1) just header
 */
static void urb_print (struct urb * urb, char * str, int small)
{
  unsigned int pipe= urb->pipe;
  char tmp_str[10];

  if (!urb->dev || !urb->dev->bus) {
    dbg("%s URB: no dev", str);
    return;
  }

  dbg("%s %p dev=%d ep=%d%s-%s flags=%x",
        str,
        urb,
        usb_pipedevice (pipe),
        usb_pipeendpoint (pipe),
        usb_pipeout (pipe)? "out" : "in",
        pipestring (pipe,tmp_str),
        urb->transfer_flags);

  dbg(" len=%d/%d stat=%d",
        urb->actual_length,
        urb->transfer_buffer_length,
        urb->status);

#ifdef  VERBOSE_DEBUG
  if (!small) {
    int i, len;

    if (usb_pipecontrol (pipe)) {
      printk (KERN_DEBUG __func__ ": setup(8):");
      for (i = 0; i < 8 ; i++)
        printk (" %02x", ((__u8 *) urb->setup_packet) [i]);
      printk ("\n");
    }
    if (urb->transfer_buffer_length > 0 && urb->transfer_buffer) {
      printk (KERN_DEBUG __func__ ": data(%d/%d):",
        urb->actual_length,
        urb->transfer_buffer_length);
      len = usb_pipeout (pipe)?
            urb->transfer_buffer_length: urb->actual_length;
      for (i = 0; i < 16 && i < len; i++)
        printk (" %02x", ((__u8 *) urb->transfer_buffer) [i]);
      printk ("%s stat:%d\n", i < len? "...": "", urb->status);
    }
  }
#endif
}

#endif



//=========================  Bruce::End ============================================