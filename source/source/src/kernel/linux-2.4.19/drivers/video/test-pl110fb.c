// --------------------------------------------------------------------
//	2002-12-05: lmc83
//		- based on sa110fb.c
// --------------------------------------------------------------------


#include <linux/config.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/init.h>

#include <asm/hardware.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/mach-types.h>
#include <asm/uaccess.h>

#include <video/fbcon.h>
#include <video/fbcon-mfb.h>
#include <video/fbcon-cfb4.h>
#include <video/fbcon-cfb8.h>
#include <video/fbcon-cfb16.h>

/*
 * debugging?
 */
#define DEBUG 1
/*
 * Complain if VAR is out of range.
 */
#define DEBUG_VAR 1


#include "pl110fb.h"

void (*pl110fb_blank_helper)(int blank);
EXPORT_SYMBOL(pl110fb_blank_helper);


#define INLINE					// inline



/// --------------------------------------------------------------------
///		和 PrimeCell PL110 相關的定義
/// --------------------------------------------------------------------
#ifndef CPE_LCD_BASE

#ifdef CONFIG_CPE120_PLATFORM
#define CPE_LCD_BASE                    0x90600000
#else
#define CPE_LCD_BASE			0x96500000
#endif

#endif
#define LCD_VA_BASE			IO_ADDRESS(CPE_LCD_BASE)


typedef struct
{
	u32 HBP: 8;
	u32 HFP: 8;
	u32 HSW: 8;
	u32 PPL: 6;
	u32 Reserve: 2;
	
}LCDTiming0;

typedef struct
{
	u32 VBP: 8;
	u32 VFP: 8;
	u32 VSW: 6;
	u32 LPP: 10;
	
}LCDTiming1;



typedef struct
{
}LCDTiming2;


typedef struct
{
	u32 Reserve0: 15;
	u32 LEE: 1;
	u32 Reserve1: 9;
	u32 LED: 7;
}LCDTiming3;



typedef struct
{
	u32 Reserve0: 15;
	u32 WATERMARK: 1;
	u32 LDmaFIFOTME: 1;
	u32 Reserve1: 2;
	u32 LcdVComp: 2;
	u32 LcdPwr: 1;
	u32 BEPO: 1;
	u32 BEBO: 1;
	u32 BGR: 1;
	u32 LcdDual: 1;
	u32 LcdMono8: 1;
	u32 LcdTFT: 1;
	u32 LcdBW: 1;
	u32 LcdBpp: 3;
	u32 LcdEn: 1;
}LCDControl;


typedef struct LCDTag
{
   	//LCDTiming0 Timing0;		// @00
    //LCDTiming1 Timing1;		// @04
    u32 Timing0;
    u32 Timing1;
    u32 Timing2;			// @08
    u32 Timing3;			// @0C
  
    u32 UPBase;				// @10
    u32 LPBase;				// @14
    u32 INTREnable;			// @18
    //LCDControl Control;		// @1C
    u32 Control;			// @1C
  
    u32 Status;				// @20
    u32 Interrupt;			// @24
    u32 UPCurr;				// @28
    u32 LPCurr;				// @2C
  
    u32 Reserved[0x74];		// @30~1FC
  
    u32 Palette[0x80];		// @200~3FC
    u32 TestReg[0x100];		// @400~7FC
    
} LCD_Register;





void INLINE Lcd_Enable (volatile LCD_Register *plcd)
{
	// LCD enable
	plcd->Control |= 0x0001;
}


void INLINE Lcd_Disable(volatile LCD_Register *plcd)
{
	// LCD disable
	plcd->Control &= ~0x0001;
}


void INLINE LcdPwr_On(volatile LCD_Register *plcd)
{
	// LCD power
	plcd->Control |= 0x0800;
}


void INLINE LcdPwr_Off(volatile LCD_Register *plcd)
{
	// LCD power
	plcd->Control &= ~0x0800;
}


void INLINE LcdUPBaseSet(volatile LCD_Register *plcd, u32 *baseaddr)
{
	// UPBase address.
	plcd->UPBase = baseaddr;
}

void INLINE LcdLPBaseSet(volatile LCD_Register *plcd, u32 *baseaddr)
{
	// UPBase address.
	plcd->LPBase = baseaddr;
}




/*
 * IMHO this looks wrong.  In 8BPP, length should be 8.
 */
static struct pl110fb_rgb rgb_8 = {
	red:	{ offset: 0,  length: 4, },
	green:	{ offset: 0,  length: 4, },
	blue:	{ offset: 0,  length: 4, },
	transp:	{ offset: 0,  length: 0, },
};


#ifdef not_complete_yet
static struct pl110fb_rgb def_rgb_16 = {
	red:	{ offset: 11, length: 5, },
	green:	{ offset: 5,  length: 6, },
	blue:	{ offset: 0,  length: 5, },
	transp:	{ offset: 0,  length: 0, },
};
#endif /* end_of_not */

static struct pl110fb_rgb def_rgb_16 = {
	transp:	{ offset: 15,  length: 1, msb_right: 0},
	blue:	{ offset: 10, length: 5, msb_right: 0},
	green:	{ offset: 5,  length: 5, msb_right: 0},
	red:	{ offset: 0,  length: 5, msb_right: 0},
};


#define LCD_FALSE			0
#define LCD_TRUE			1

/*
 * The assabet uses a sharp LQ039Q2DS54 LCD module.  It is actually
 * takes an RGB666 signal, but we provide it with an RGB565 signal
 * instead (def_rgb_16).
 */
static struct pl110fb_mach_info Sharp_LQ084C1DG21_info __initdata = 
{
	time0:
     	 ((44			- 1) << 24) //Horizontal back porch
		|((20			- 1) << 16)	//Horizontal front porch
		|((96			- 1) << 8 )	//Horizontal Sync. pulse width
		|(((640 >> 4)	- 1) << 2 ),//pixels-per-line = 16(PPL+1)

	time1:
		 (34		<< 24)			//Vertical back porch
		|(10		<< 16) 			//Vertical front porch
		|((1 - 1)	<< 10) 			//Vertical Sync. pulse width
		|((480 - 1)		 ),			//lines-per-panel = LPP+1
		
	time2:
		 (LCD_FALSE	<< 26) 			//Bypass pixel clock divider
		|((640 - 1)	<< 16) 			//Clock per line
		|(LCD_FALSE	<< 14) 			//invert output enable
		|(LCD_FALSE	<< 13) 			//invert panel clock
		|(LCD_TRUE	<< 12) 			//invert horizontal sync.
		|(LCD_TRUE	<< 11) 			//invert vertical sync
		|((32 - 1)	<<  6) 			//ac bias
		|(LCD_FALSE	<<  5) 			//clock select
		|((8 - 2   )&0x1f),			//panel clock divdsor

	control:
	 	 (LCD_FALSE	<< 16) 			//LCD DMA FIFO watermark level
		|(LCD_FALSE	<< 15) 			//LCD DMA FIFO test mode enable
		|(0			<< 12) 			//LcdVComp, when to generate interrupt
		|(LCD_FALSE	<< 11) 			//LCD power enable
		|(LCD_FALSE	<< 10) 			//Big-endian pixel ordering
		|(LCD_FALSE	<<  9) 			//Big-endian Byte ordering
		|(LCD_FALSE	<<  8) 			//BGR
		|(LCD_FALSE	<<  7) 			//LcdDual
		|(LCD_FALSE	<<  6) 			//LcdMono8
		|(LCD_TRUE	<<  5) 			//LcdTFT
		|(LCD_FALSE	<<  4) 			//LcdBW
		|(4			<<  1) 			//LCD bits per pixel: 16bpp
		|(LCD_FALSE		 ),			//LCD controller enable


	
	pixclock:		171521,
	xres:			640,		
	yres:			480,
	bpp:			16,

#ifdef not_complete_yet
	hsync_len:		96,
	left_margin:	44,		
	right_margin:	20,

	vsync_len:		1,
	upper_margin:	3,
	lower_margin:	0,
#endif /* end_of_not */


	sync:		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
};


// --------------------------------------------------------------------
//		V/Q=Low
// --------------------------------------------------------------------
static struct pl110fb_mach_info Sharp_LQ057Q3DC02_info __initdata = 
{
	time0:
     	 ((2			- 1) << 24) //Horizontal back porch
		|((48			- 1) << 16)	//Horizontal front porch
		|((70			- 1) << 8 )	//Horizontal Sync. pulse width		
		|(((640 >> 4)	- 1) << 2 ),//pixels-per-line = 16(PPL+1)

	time1:
		 (7 		<< 24)			//Vertical back porch
		|(15		<< 16) 			//Vertical front porch
		|((1 - 1)	<< 10) 			//Vertical Sync. pulse width
		|((240 - 1)		 ),			//lines-per-panel = LPP+1
		
	time2:
		 (LCD_FALSE	<< 26) 			//Bypass pixel clock divider
		|((640 - 1)	<< 16) 			//Clock per line
		|(LCD_FALSE	<< 14) 			//invert output enable
		|(LCD_FALSE	<< 13) 			//invert panel clock
		|(LCD_TRUE	<< 12) 			//invert horizontal sync.
		|(LCD_TRUE	<< 11) 			//invert vertical sync
		|((32 - 1)	<<  6) 			//ac bias
		|(LCD_FALSE	<<  5) 			//clock select
		//|((4 - 2   )&0x1f),			//panel clock divdsor
		//ivan (testchip version)
		|(6&0x1f),

	control:
	 	 (LCD_FALSE	<< 16) 			//LCD DMA FIFO watermark level
		|(LCD_FALSE	<< 15) 			//LCD DMA FIFO test mode enable
		|(0			<< 12) 			//LcdVComp, when to generate interrupt
		|(LCD_FALSE	<< 11) 			//LCD power enable
		|(LCD_FALSE	<< 10) 			//Big-endian pixel ordering
		|(LCD_FALSE	<<  9) 			//Big-endian Byte ordering
		|(LCD_FALSE	<<  8) 			//BGR
		|(LCD_FALSE	<<  7) 			//LcdDual
		|(LCD_FALSE	<<  6) 			//LcdMono8
		|(LCD_TRUE	<<  5) 			//LcdTFT
		|(LCD_FALSE	<<  4) 			//LcdBW
		|(4			<<  1) 			//LCD bits per pixel: 16bpp
		|(LCD_FALSE		 ),			//LCD controller enable


	
	pixclock:		171521,
	xres:			320,		
	yres:			240,
	bpp:			16,

#ifdef not_complete_yet
	hsync_len:		96,
	left_margin:	44,		
	right_margin:	20,

	vsync_len:		1,
	upper_margin:	3,
	lower_margin:	0,
#endif /* end_of_not */


	sync:		FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
};




static struct pl110fb_mach_info * __init pl110fb_get_machine_info(struct pl110fb_info *fbi)
{
	struct pl110fb_mach_info *inf = NULL;

	/*
	 *            R        G       B       T
	 * default  {11,5}, { 5,6}, { 0,5}, { 0,0}
	 * h3600    {12,4}, { 7,4}, { 1,4}, { 0,0}
	 * freebird { 8,4}, { 4,4}, { 0,4}, {12,4}
	 */
	///inf = &Sharp_LQ084C1DG21_info;
	inf = &Sharp_LQ057Q3DC02_info;

	return inf;
}





static int pl110fb_activate_var(struct fb_var_screeninfo *var, struct pl110fb_info *);
static void set_ctrlr_state(struct pl110fb_info *fbi, u_int state);


static INLINE void pl110fb_schedule_task(struct pl110fb_info *fbi, u_int state)
{
	unsigned long flags;

	local_irq_save(flags);
	/*
	 * We need to handle two requests being made at the same time.
	 * There are two important cases:
	 *  1. When we are changing VT (C_REENABLE) while unblanking (C_ENABLE)
	 *     We must perform the unblanking, which will do our REENABLE for us.
	 *  2. When we are blanking, but immediately unblank before we have
	 *     blanked.  We do the "REENABLE" thing here as well, just to be sure.
	 */
	if (fbi->task_state == C_ENABLE && state == C_REENABLE)
		state = (u_int) -1;
	if (fbi->task_state == C_DISABLE && state == C_ENABLE)
		state = C_REENABLE;

	if (state != (u_int)-1)				/// 需要改變 state
	{
		fbi->task_state = state;
		schedule_task(&fbi->task);
	}
	local_irq_restore(flags);
}



/*
 * Get the VAR structure pointer for the specified console
 */
static INLINE struct fb_var_screeninfo *get_con_var(struct fb_info *info, int con)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	return (con == fbi->currcon || con == -1) ? &fbi->fb.var : &fb_display[con].var;
}

/*
 * Get the DISPLAY structure pointer for the specified console
 */
static INLINE struct display *get_con_display(struct fb_info *info, int con)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	return (con < 0) ? fbi->fb.disp : &fb_display[con];
}

/*
 * Get the CMAP pointer for the specified console
 */
static INLINE struct fb_cmap *get_con_cmap(struct fb_info *info, int con)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	return (con == fbi->currcon || con == -1) ? &fbi->fb.cmap : &fb_display[con].cmap;
}


/// --------------------------------------------------------------------
///		將 chan 依定指定的 bitfield, 填到 bit 適當的位置
/// --------------------------------------------------------------------
static INLINE u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
	chan &= 0xffff;
	chan >>= 16 - bf->length;
	return chan << bf->offset;
}



/*
 * Convert bits-per-pixel to a hardware palette PBS value.
 */
static INLINE u_int palette_pbs(struct fb_var_screeninfo *var)
{
	int ret = 0;
	
#ifdef not_complete_yet
	switch (var->bits_per_pixel) {
#ifdef FBCON_HAS_CFB4
	case 4:  ret = 0 << 12;	break;
#endif
#ifdef FBCON_HAS_CFB8
	case 8:  ret = 1 << 12; break;
#endif
#ifdef FBCON_HAS_CFB16
	case 16: ret = 2 << 12; break;
#endif
	}
#endif /* end_of_not */
	return ret;
}




static int pl110fb_setpalettereg(u_int regno, u_int red, u_int green, u_int blue,
		       u_int trans, struct fb_info *info)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	u_int val, ret = 1;

#ifdef not_complete_yet
	if (regno < fbi->palette_size) {
		val = ((red >> 4) & 0xf00);
		val |= ((green >> 8) & 0x0f0);
		val |= ((blue >> 12) & 0x00f);

		if (regno == 0)
			val |= palette_pbs(&fbi->fb.var);

		fbi->palette_cpu[regno] = val;
		ret = 0;
	}
#endif /* end_of_not */
	return ret;
}




static int pl110fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
		   u_int trans, struct fb_info *info)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	struct display *disp = get_con_display(info, fbi->currcon);
	u_int val;
	int ret = 1;


	/*
	 * If inverse mode was selected, invert all the colours
	 * rather than the register number.  The register number
	 * is what you poke into the framebuffer to produce the
	 * colour you requested.
	 */
	if (disp->inverse) 
	{
		BUG();						/// not complete yet
		red   = 0xffff - red;
		green = 0xffff - green;
		blue  = 0xffff - blue;
	}

	/*
	 * If greyscale is true, then we convert the RGB value
	 * to greyscale no mater what visual we are using.
	 */
	if (fbi->fb.var.grayscale)
	{
		BUG();						/// not complete yet
		red = green = blue = (19595 * red + 38470 * green +	7471 * blue) >> 16;
	}

	switch (fbi->fb.disp->visual) 
	{
		case FB_VISUAL_TRUECOLOR:
			/*
		 	 * 12 or 16-bit True Colour.  We encode the RGB value
		 	 * according to the RGB bitfield information.
		 	*/
			if (regno < 16) 
			{
				u16 *pal = fbi->fb.pseudo_palette;

				val  = chan_to_field(red, &fbi->fb.var.red);
				val |= chan_to_field(green, &fbi->fb.var.green);
				val |= chan_to_field(blue, &fbi->fb.var.blue);

				pal[regno] = val;
				ret = 0;
			}
			break;

		case FB_VISUAL_STATIC_PSEUDOCOLOR:
		case FB_VISUAL_PSEUDOCOLOR:
			ret = pl110fb_setpalettereg(regno, red, green, blue, trans, info);
			break;
	}

	return ret;
}


/*
 *  pl110fb_display_dma_period()
 *    Calculate the minimum period (in picoseconds) between two DMA
 *    requests for the LCD controller.
 */
static unsigned int pl110fb_display_dma_period(struct fb_var_screeninfo *var)
{
	unsigned int mem_bits_per_pixel;

	mem_bits_per_pixel = var->bits_per_pixel;
	if (mem_bits_per_pixel == 12)
		mem_bits_per_pixel = 16;

	/*
	 * Period = pixclock * bits_per_byte * bytes_per_transfer
	 *		/ memory_bits_per_pixel;
	 */
	return var->pixclock * 8 * 16 / mem_bits_per_pixel;
}



/*
 *  pl110fb_decode_var():
 *    Get the video params out of 'var'. If a value doesn't fit, round it up,
 *    if it's too big, return -EINVAL.
 *
 *    Suggestion: Round up in the following order: bits_per_pixel, xres,
 *    yres, xres_virtual, yres_virtual, xoffset, yoffset, grayscale,
 *    bitfields, horizontal timing, vertical timing.
 */
/// --------------------------------------------------------------------
///		將 var 裏的 xres, yres,.. 做適當的調整, 使其在合理的範圍值內
///		return -EINVAL ==> error
///				other ==> ok
/// --------------------------------------------------------------------
static int pl110fb_validate_var(struct fb_var_screeninfo *var, struct pl110fb_info *fbi)
{
	int ret = -EINVAL;

	if (var->xres < MIN_XRES)
		var->xres = MIN_XRES;
	if (var->yres < MIN_YRES)
		var->yres = MIN_YRES;
	if (var->xres > fbi->max_xres)
		var->xres = fbi->max_xres;
	if (var->yres > fbi->max_yres)
		var->yres = fbi->max_yres;
	var->xres_virtual =
	    var->xres_virtual < var->xres ? var->xres : var->xres_virtual;
	var->yres_virtual =
	    var->yres_virtual < var->yres ? var->yres : var->yres_virtual;

	DPRINTK("var->bits_per_pixel=%d\n", var->bits_per_pixel);
	switch (var->bits_per_pixel) 
	{
#ifdef FBCON_HAS_CFB4
		case 4:		ret = 0; break;
#endif
#ifdef FBCON_HAS_CFB8
		case 8:		ret = 0; break;
#endif
#ifdef FBCON_HAS_CFB16
		case 16:	ret = 0; break;
#endif
	default:
		printk("not support bpp: %d\n", var->bits_per_pixel);
		break;
	}


	return ret;
}


static INLINE void pl110fb_set_truecolor(u_int is_true_color)
{

}


/// --------------------------------------------------------------------
///		將 var 裏的設定值反應到 hardware 上去
/// --------------------------------------------------------------------
static void pl110fb_hw_set_var(struct fb_var_screeninfo *var, struct pl110fb_info *fbi)
{
	volatile LCD_Register *plcd = (LCD_Register *)fbi->io_base;
	
	plcd->Timing0 = fbi->time0;
	plcd->Timing1 = fbi->time1;
	plcd->Timing2 = fbi->time2;
	plcd->Control = fbi->control;
	
	plcd->UPBase = fbi->fb.fix.smem_start;

	
#ifdef not_complete_yet
	plcd->Timing0.HBP = var->left_margin - 1;
	plcd->Timing0.HFP = var->right_margin - 1;
	plcd->Timing0.HSW = var->hsync_len - 1;
	plcd->Timing0.PPL = ((var->xres >> 4)-1) * 16;
	
	plcd->Timing1.VBP = var->upper_margin;
	plcd->Timing1.VFP = var->lower_margin;
	plcd->Timing1.VSW = var->vsync_len - 1;
	plcd->Timing1.LPP = var->yres - 1;
#endif /* end_of_not */

	
#ifdef not_complete_yet
	u_long palette_mem_size;

	fbi->palette_size = var->bits_per_pixel == 8 ? 256 : 16;

	palette_mem_size = fbi->palette_size * sizeof(u16);

	DPRINTK("palette_mem_size = 0x%08lx\n", (u_long) palette_mem_size);

	fbi->palette_cpu = (u16 *)(fbi->map_cpu + PAGE_SIZE - palette_mem_size);
	fbi->palette_dma = fbi->map_dma + PAGE_SIZE - palette_mem_size;
#endif /* end_of_not */

	fb_set_cmap(&fbi->fb.cmap, 1, pl110fb_setcolreg, &fbi->fb);

#ifdef not_complete_yet
	/* Set board control register to handle new color depth */
	pl110fb_set_truecolor(var->bits_per_pixel >= 16);

	pl110fb_activate_var(var, fbi);

	fbi->palette_cpu[0] = (fbi->palette_cpu[0] & 0xcfff) | palette_pbs(var);
#endif /* end_of_not */
}

/*
 * pl110fb_set_var():
 *	Set the user defined part of the display for the specified console
 */
/// --------------------------------------------------------------------
///		修改指定的 console (con) 的 var
/// --------------------------------------------------------------------
static int pl110fb_set_var(struct fb_var_screeninfo *var, int con, struct fb_info *info)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	struct fb_var_screeninfo *dvar = get_con_var(&fbi->fb, con);			/// 取出 con 的目前 var 設定
	struct display *display = get_con_display(&fbi->fb, con);				
	int err, chgvar = 0, rgbidx;

	DPRINTK("set_var\n");

	/*
	 * Decode var contents into a par structure, adjusting any
	 * out of range values.
	 */
	err = pl110fb_validate_var(var, fbi);
	if (err)
	{
		printk("invalid var setting\n");
		return err;
	}

	if (var->activate & FB_ACTIVATE_TEST)
		return 0;

	if ((var->activate & FB_ACTIVATE_MASK) != FB_ACTIVATE_NOW)
		return -EINVAL;

	/// --------------------------------------------------------------------
	///		檢查 xres, yres,.. 是否有改變, 若有, 則 chgvar = 1
	/// --------------------------------------------------------------------
	if (dvar->xres != var->xres)
		chgvar = 1;
	if (dvar->yres != var->yres)
		chgvar = 1;
	if (dvar->xres_virtual != var->xres_virtual)
		chgvar = 1;
	if (dvar->yres_virtual != var->yres_virtual)
		chgvar = 1;
	if (dvar->bits_per_pixel != var->bits_per_pixel)
		chgvar = 1;
	if (con < 0)
		chgvar = 0;


	switch (var->bits_per_pixel)
	{
#ifdef FBCON_HAS_CFB4
	case 4:
		if (fbi->cmap_static)
			display->visual	= FB_VISUAL_STATIC_PSEUDOCOLOR;
		else
			display->visual	= FB_VISUAL_PSEUDOCOLOR;
		display->line_length	= var->xres / 2;
		display->dispsw		= &fbcon_cfb4;
		rgbidx			= RGB_8;
		break;
#endif
#ifdef FBCON_HAS_CFB8
	case 8:
		if (fbi->cmap_static)
			display->visual	= FB_VISUAL_STATIC_PSEUDOCOLOR;
		else
			display->visual	= FB_VISUAL_PSEUDOCOLOR;
		display->line_length	= var->xres;
		display->dispsw		= &fbcon_cfb8;
		rgbidx			= RGB_8;
		break;
#endif
#ifdef FBCON_HAS_CFB16
	case 16:
		display->visual			= FB_VISUAL_TRUECOLOR;
		display->line_length	= var->xres * 2;			/// line_length ==> 螢幕上的每一行佔多少個 byte
		display->dispsw			= &fbcon_cfb16;
		display->dispsw_data	= fbi->fb.pseudo_palette;
		rgbidx					= RGB_16;
		break;
#endif
	default:
		rgbidx = 0;
		display->dispsw = &fbcon_dummy;
		break;
	}

	display->screen_base	= fbi->screen_cpu;
	display->next_line		= display->line_length;
	display->type			= fbi->fb.fix.type;
	display->type_aux		= fbi->fb.fix.type_aux;
	display->ypanstep		= fbi->fb.fix.ypanstep;
	display->ywrapstep		= fbi->fb.fix.ywrapstep;
	display->can_soft_blank	= 1;
	display->inverse		= fbi->cmap_inverse;

	*dvar					= *var;
	dvar->activate			&= ~FB_ACTIVATE_ALL;

	/*
	 * Copy the RGB parameters for this display
	 * from the machine specific parameters.
	 */
	dvar->red			= fbi->rgb[rgbidx]->red;
	dvar->green			= fbi->rgb[rgbidx]->green;
	dvar->blue			= fbi->rgb[rgbidx]->blue;
	dvar->transp		= fbi->rgb[rgbidx]->transp;

	DPRINTK("RGBT length = %d:%d:%d:%d\n",
		dvar->red.length, dvar->green.length, dvar->blue.length,
		dvar->transp.length);

	DPRINTK("RGBT offset = %d:%d:%d:%d\n",
		dvar->red.offset, dvar->green.offset, dvar->blue.offset,
		dvar->transp.offset);

	/*
	 * Update the old var.  The fbcon drivers still use this.
	 * Once they are using fbi->fb.var, this can be dropped.
	 */
	display->var = *dvar;

	/*
	 * If we are setting all the virtual consoles, also set the
	 * defaults used to create new consoles.
	 */
	if (var->activate & FB_ACTIVATE_ALL)
		fbi->fb.disp->var = *dvar;

	/*
	 * If the console has changed and the console has defined
	 * a changevar function, call that function.
	 */
	if (chgvar && info && fbi->fb.changevar)
		fbi->fb.changevar(con);

	/* If the current console is selected, activate the new var. */
	if (con != fbi->currcon)
		return 0;

	pl110fb_hw_set_var(dvar, fbi);					/// 將修改的設定反應到 hardware

	return 0;
}


static int __do_set_cmap(struct fb_cmap *cmap, int kspc, int con, struct fb_info *info)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	struct fb_cmap *dcmap = get_con_cmap(info, con);
	int err = 0;

	if (con == -1)
		con = fbi->currcon;

	/* no colormap allocated? (we always have "this" colour map allocated) */
	if (con >= 0)
		err = fb_alloc_cmap(&fb_display[con].cmap, fbi->palette_size, 0);

	if (!err && con == fbi->currcon)
		err = fb_set_cmap(cmap, kspc, pl110fb_setcolreg, info);

	if (!err)
		fb_copy_cmap(cmap, dcmap, kspc ? 0 : 1);

	return err;
}

static int pl110fb_set_cmap(struct fb_cmap *cmap, int kspc, int con, struct fb_info *info)
{
	struct display *disp = get_con_display(info, con);

	if (disp->visual == FB_VISUAL_TRUECOLOR ||
	    disp->visual == FB_VISUAL_STATIC_PSEUDOCOLOR)
		return -EINVAL;

	return __do_set_cmap(cmap, kspc, con, info);
}


static int pl110fb_get_fix(struct fb_fix_screeninfo *fix, int con, struct fb_info *info)
{
	struct display *display = get_con_display(info, con);

	*fix = info->fix;

	fix->line_length = display->line_length;
	fix->visual	 = display->visual;
	return 0;
}


static int pl110fb_get_var(struct fb_var_screeninfo *var, int con, struct fb_info *info)
{
	*var = *get_con_var(info, con);
	return 0;
}


static int pl110fb_get_cmap(struct fb_cmap *cmap, int kspc, int con, struct fb_info *info)
{
	struct fb_cmap *dcmap = get_con_cmap(info, con);
	fb_copy_cmap(dcmap, cmap, kspc ? 0 : 2);
	return 0;
}

static struct fb_ops pl110fb_ops = {
	owner:			THIS_MODULE,
	fb_get_fix:		pl110fb_get_fix,
	fb_get_var:		pl110fb_get_var,
	fb_set_var:		pl110fb_set_var,
	fb_get_cmap:	pl110fb_get_cmap,
	fb_set_cmap:	pl110fb_set_cmap,
};


/*
 *  pl110fb_switch():       
 *	Change to the specified console.  Palette and video mode
 *      are changed to the console's stored parameters.
 *
 *	Uh oh, this can be called from a tasklet (IRQ)
 */
static int pl110fb_switch(int con, struct fb_info *info)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	struct display *disp;
	struct fb_cmap *cmap;

	DPRINTK("con=%d info->modename=%s\n", con, fbi->fb.modename);

	if (con == fbi->currcon)
		return 0;

	if (fbi->currcon >= 0) 				/// 先將 current console 的內容存起來
	{
		disp = fb_display + fbi->currcon;

		/*
		 * Save the old colormap and video mode.
		 */
		disp->var = fbi->fb.var;

		if (disp->cmap.len)
			fb_copy_cmap(&fbi->fb.cmap, &disp->cmap, 0);
	}


	fbi->currcon = con;
	disp = fb_display + con;

	/*
	 * Make sure that our colourmap contains 256 entries.
	 */
	fb_alloc_cmap(&fbi->fb.cmap, 256, 0);

	if (disp->cmap.len)
		cmap = &disp->cmap;
	else
		cmap = fb_default_cmap(1 << disp->var.bits_per_pixel);

	fb_copy_cmap(cmap, &fbi->fb.cmap, 0);

	fbi->fb.var = disp->var;
	fbi->fb.var.activate = FB_ACTIVATE_NOW;

	pl110fb_set_var(&fbi->fb.var, con, info);
	return 0;
}



/*
 * Formal definition of the VESA spec:
 *  On
 *  	This refers to the state of the display when it is in full operation
 *  Stand-By
 *  	This defines an optional operating state of minimal power reduction with
 *  	the shortest recovery time
 *  Suspend
 *  	This refers to a level of power management in which substantial power
 *  	reduction is achieved by the display.  The display can have a longer 
 *  	recovery time from this state than from the Stand-by state
 *  Off
 *  	This indicates that the display is consuming the lowest level of power
 *  	and is non-operational. Recovery from this state may optionally require
 *  	the user to manually power on the monitor
 *
 *  Now, the fbdev driver adds an additional state, (blank), where they
 *  turn off the video (maybe by colormap tricks), but don't mess with the
 *  video itself: think of it semantically between on and Stand-By.
 *
 *  So here's what we should do in our fbdev blank routine:
 *
 *  	VESA_NO_BLANKING (mode 0)	Video on,  front/back light on
 *  	VESA_VSYNC_SUSPEND (mode 1)  	Video on,  front/back light off
 *  	VESA_HSYNC_SUSPEND (mode 2)  	Video on,  front/back light off
 *  	VESA_POWERDOWN (mode 3)		Video off, front/back light off
 *
 *  This will match the matrox implementation.
 */
/*
 * pl110fb_blank():
 *	Blank the display by setting all palette values to zero.  Note, the 
 * 	12 and 16 bpp modes don't really use the palette, so this will not
 *      blank the display in all modes.  
 */
static void pl110fb_blank(int blank, struct fb_info *info)
{
	struct pl110fb_info *fbi = (struct pl110fb_info *)info;
	int i;

	DPRINTK("pl110fb_blank: blank=%d info->modename=%s\n", blank,	fbi->fb.modename);

#ifdef not_complete_yet
	switch (blank) {
	case VESA_POWERDOWN:
	case VESA_VSYNC_SUSPEND:
	case VESA_HSYNC_SUSPEND:
		if (fbi->fb.disp->visual == FB_VISUAL_PSEUDOCOLOR ||
		    fbi->fb.disp->visual == FB_VISUAL_STATIC_PSEUDOCOLOR)
			for (i = 0; i < fbi->palette_size; i++)
				pl110fb_setpalettereg(i, 0, 0, 0, 0, info);
		pl110fb_schedule_task(fbi, C_DISABLE);
		if (pl110fb_blank_helper)
			pl110fb_blank_helper(blank);
		break;

	case VESA_NO_BLANKING:
		if (pl110fb_blank_helper)
			pl110fb_blank_helper(blank);
		if (fbi->fb.disp->visual == FB_VISUAL_PSEUDOCOLOR ||
		    fbi->fb.disp->visual == FB_VISUAL_STATIC_PSEUDOCOLOR)
			fb_set_cmap(&fbi->fb.cmap, 1, pl110fb_setcolreg, info);
		pl110fb_schedule_task(fbi, C_ENABLE);
	}
#endif /* end_of_not */
}

static int pl110fb_updatevar(int con, struct fb_info *info)
{
	DPRINTK("entered\n");
	return 0;
}



/*
 * Calculate the PCD value from the clock rate (in picoseconds).
 * We take account of the PPCR clock setting.
 */
static INLINE int get_pcd(unsigned int pixclock)
{
	unsigned int pcd;
	
	pcd = 0;
#ifdef not_complete_yet
	if (pixclock) {
		pcd = get_cclk_frequency() * pixclock;
		pcd /= 10000000;
		pcd += 1;	/* make up for integer math truncations */
	} else {
		/*
		 * People seem to be missing this message.  Make it big.
		 * Make it stand out.  Make sure people see it.
		 */
		printk(KERN_WARNING "******************************************************\n");
		printk(KERN_WARNING "**            ZERO PIXEL CLOCK DETECTED             **\n");
		printk(KERN_WARNING "** You are using a zero pixclock.  This means that  **\n");
		printk(KERN_WARNING "** clock scaling will not be able to adjust your    **\n");
		printk(KERN_WARNING "** your timing parameters appropriately, and the    **\n");
		printk(KERN_WARNING "** bandwidth calculations will fail to work.  This  **\n");
		printk(KERN_WARNING "** will shortly become an error condition, which    **\n");
		printk(KERN_WARNING "** will prevent your LCD display working.  Please   **\n");
		printk(KERN_WARNING "** send your patches in as soon as possible to shut **\n");
		printk(KERN_WARNING "** this message up.                                 **\n");
		printk(KERN_WARNING "******************************************************\n");
		pcd = 0;
	}
#endif /* end_of_not */
	
	return pcd;
}




/*
 * pl110fb_activate_var():
 *	Configures LCD Controller based on entries in var parameter.  Settings are      
 *	only written to the controller if changes were made.  
 */
/// --------------------------------------------------------------------
///		讓 var 的設定做用到 LCD controller
/// -------------------------------------------------------------------- 
static int pl110fb_activate_var(struct fb_var_screeninfo *var, struct pl110fb_info *fbi)
{
	struct pl110fb_lcd_reg new_regs;
	u_int half_screen_size, yres, pcd = get_pcd(var->pixclock);
	u_long flags;

	DPRINTK("Configuring PL110 LCD\n");

	DPRINTK("var: xres=%d hslen=%d lm=%d rm=%d\n",
		var->xres, var->hsync_len,
		var->left_margin, var->right_margin);
	DPRINTK("var: yres=%d vslen=%d um=%d bm=%d\n",
		var->yres, var->vsync_len,
		var->upper_margin, var->lower_margin);

	/*
	 * If we have a dual scan LCD, then we need to halve
	 * the YRES parameter.
	 */
	yres = var->yres;

#ifdef not_complete_yet	
	if (fbi->lccr0 & LCCR0_Dual)
		yres /= 2;

	new_regs.lccr2 =
		LCCR2_DisHght(yres) +
		LCCR2_VrtSnchWdth(var->vsync_len) +
		LCCR2_BegFrmDel(var->upper_margin) +
		LCCR2_EndFrmDel(var->lower_margin);

	new_regs.lccr3 = fbi->lccr3 |
		(var->sync & FB_SYNC_HOR_HIGH_ACT ? LCCR3_HorSnchH : LCCR3_HorSnchL) |
		(var->sync & FB_SYNC_VERT_HIGH_ACT ? LCCR3_VrtSnchH : LCCR3_VrtSnchL) |
		LCCR3_ACBsCntOff;


	if (pcd)
		new_regs.lccr3 |= LCCR3_PixClkDiv(pcd);
#endif /* end_of_not */


	half_screen_size = var->bits_per_pixel;
	half_screen_size = half_screen_size * var->xres * var->yres / 16;

#ifdef not_complete_yet
	/* Update shadow copy atomically */
	local_irq_save(flags);
	fbi->dbar1 = fbi->palette_dma;
	fbi->dbar2 = fbi->screen_dma + half_screen_size;

	local_irq_restore(flags);
#endif /* end_of_not */


	/*
	 * Only update the registers if the controller is enabled
	 * and something has changed.
	 */
#ifdef not_complete_yet	 
	if ((LCCR0 != fbi->reg_lccr0)       || (LCCR1 != fbi->reg_lccr1) ||
	    (LCCR2 != fbi->reg_lccr2)       || (LCCR3 != fbi->reg_lccr3) ||
	    (DBAR1 != fbi->dbar1) || (DBAR2 != fbi->dbar2))
#endif /* end_of_not */
	{
		pl110fb_schedule_task(fbi, C_REENABLE);
	}

	return 0;
}


/*
 * NOTE!  The following functions are purely helpers for set_ctrlr_state.
 * Do not call them directly; set_ctrlr_state does the correct serialisation
 * to ensure that things happen in the right way 100% of time time.
 *	-- rmk
 */

static void pl110fb_power_up_lcd(struct pl110fb_info *fbi)
{
	volatile LCD_Register *plcd = (LCD_Register *)fbi->io_base;
	
	DPRINTK("LCD power on\n");
	plcd->Control |= 0x0800;
}

static void pl110fb_power_down_lcd(struct pl110fb_info *fbi)
{
	volatile LCD_Register *plcd = (LCD_Register *)fbi->io_base;
	
	DPRINTK("LCD power off\n");
	plcd->Control &= ~0x0800;
}


static void pl110fb_enable_controller(struct pl110fb_info *fbi)
{
	volatile LCD_Register *plcd = (LCD_Register *)fbi->io_base;
	
	DPRINTK("Enabling LCD controller\n");
	plcd->Control |= 0x0001;
}


static void pl110fb_disable_controller(struct pl110fb_info *fbi)
{
	DECLARE_WAITQUEUE(wait, current);

	DPRINTK("Disabling LCD controller\n");
	BUG();							/// not complete yet

	add_wait_queue(&fbi->ctrlr_wait, &wait);
	set_current_state(TASK_UNINTERRUPTIBLE);

#ifdef not_complete_yet
	LCSR = 0xffffffff;	/* Clear LCD Status Register */
	LCCR0 &= ~LCCR0_LDM;	/* Enable LCD Disable Done Interrupt */
	enable_irq(IRQ_LCD);	/* Enable LCD IRQ */
	LCCR0 &= ~LCCR0_LEN;	/* Disable LCD Controller */
#endif /* end_of_not */

	schedule_timeout(20 * HZ / 1000);
	current->state = TASK_RUNNING;
	remove_wait_queue(&fbi->ctrlr_wait, &wait);
}

/*
 *  pl110fb_handle_irq: Handle 'LCD DONE' interrupts.
 */
static void pl110fb_handle_irq(int irq, void *dev_id, struct pt_regs *regs)
{
#ifdef not_complete_yet	
	struct pl110fb_info *fbi = dev_id;
	unsigned int lcsr = LCSR;

	if (lcsr & LCSR_LDD) {
		LCCR0 |= LCCR0_LDM;
		wake_up(&fbi->ctrlr_wait);
	}

	LCSR = lcsr;
#endif /* end_of_not */
}

/*
 * This function must be called from task context only, since it will
 * sleep when disabling the LCD controller, or if we get two contending
 * processes trying to alter state.
 */
static void set_ctrlr_state(struct pl110fb_info *fbi, u_int state)
{
	u_int old_state;

	down(&fbi->ctrlr_sem);

	old_state = fbi->state;

	switch (state) 
	{
		case C_DISABLE:
			/*
		 	 * Disable controller
		 	 */
			if (old_state != C_DISABLE) 
			{
				fbi->state = state;

				pl110fb_disable_controller(fbi);
				pl110fb_power_down_lcd(fbi);
			}
			break;

		case C_REENABLE:					/// 當有修改 controller 的設定時, 才需要
			/*
		 	 * Re-enable the controller only if it was already
		 	 * enabled.  This is so we reprogram the control
		 	 * registers.
		 	 */
			if (old_state == C_ENABLE)
			{
				pl110fb_disable_controller(fbi);
				pl110fb_enable_controller(fbi);
			}
			break;

		case C_ENABLE:
			/*
		 	 * Power up the LCD screen, enable controller, and
		 	 * turn on the backlight.
		 	 */
			if (old_state != C_ENABLE) 
			{
				fbi->state = C_ENABLE;
				pl110fb_power_up_lcd(fbi);
				pl110fb_enable_controller(fbi);
			}
			break;
	}
	up(&fbi->ctrlr_sem);
}

/*
 * Our LCD controller task (which is called when we blank or unblank)
 * via keventd.
 */
static void pl110fb_task(void *dummy)
{
	struct pl110fb_info *fbi = dummy;
	u_int state = xchg(&fbi->task_state, -1);

	set_ctrlr_state(fbi, state);
}




/*
 * pl110fb_map_video_memory():
 *      Allocates the DRAM memory for the frame buffer.  This buffer is  
 *	remapped into a non-cached, non-buffered, memory region to  
 *      allow palette and pixel writes to occur without flushing the 
 *      cache.  Once this area is remapped, all virtual memory
 *      access to the video memory should occur at the new region.
 */
static int __init pl110fb_map_video_memory(struct pl110fb_info *fbi)
{

	/*
	 * We reserve one page for the palette, plus the size
	 * of the framebuffer.
	 */
	fbi->map_size = PAGE_ALIGN(fbi->fb.fix.smem_len + PAGE_SIZE);

/*	fbi->map_cpu = kmalloc( fbi->map_size, GFP_DMA|GFP_KERNEL );
	fbi->map_dma = virt_to_phys(fbi->map_cpu);*/
	fbi->map_cpu = consistent_alloc(GFP_KERNEL, fbi->map_size, &fbi->map_dma);

	if (fbi->map_cpu)
	{
		memset(fbi->map_cpu, 0xff, fbi->map_size);
		
		fbi->screen_cpu = fbi->map_cpu + PAGE_SIZE;
		fbi->screen_dma = fbi->map_dma + PAGE_SIZE;
		fbi->fb.fix.smem_start = fbi->screen_dma;
	}


	return fbi->map_cpu ? 0 : -ENOMEM;

}

/* Fake monspecs to fill in fbinfo structure */
static struct fb_monspecs monspecs __initdata = {
	30000, 70000, 50, 65, 0	/* Generic */
};



// --------------------------------------------------------------------
//	allocate and initialized pl110fb_info 資料結構
// --------------------------------------------------------------------
static struct pl110fb_info * __init pl110fb_init_fbinfo(void)
{
	struct pl110fb_mach_info *inf;
	struct pl110fb_info *fbi;

	fbi = kmalloc(sizeof(struct pl110fb_info) + sizeof(struct display) +
		      sizeof(u16) * 16, GFP_KERNEL);
	if (!fbi)
		return NULL;

	memset(fbi, 0, sizeof(struct pl110fb_info) + sizeof(struct display));

	fbi->currcon		= -1;
	//fbi->io_base 		= LCD_BASE;
	fbi->io_base 		= LCD_VA_BASE;

	strcpy(fbi->fb.fix.id, PL110_NAME);

	fbi->fb.fix.type	= FB_TYPE_PACKED_PIXELS;
	fbi->fb.fix.type_aux	= 0;
	fbi->fb.fix.xpanstep	= 0;
	fbi->fb.fix.ypanstep	= 0;
	fbi->fb.fix.ywrapstep	= 0;
	fbi->fb.fix.accel	= FB_ACCEL_NONE;

	fbi->fb.var.nonstd	= 0;
	fbi->fb.var.activate	= FB_ACTIVATE_NOW;
	fbi->fb.var.height	= -1;
	fbi->fb.var.width	= -1;
	fbi->fb.var.accel_flags	= 0;
	fbi->fb.var.vmode	= FB_VMODE_NONINTERLACED;

	strcpy(fbi->fb.modename, PL110_NAME);
	strcpy(fbi->fb.fontname, "Acorn8x8");

	fbi->fb.fbops		= &pl110fb_ops;
	fbi->fb.changevar	= NULL;
	fbi->fb.switch_con	= pl110fb_switch;
	fbi->fb.updatevar	= pl110fb_updatevar;
	fbi->fb.blank		= pl110fb_blank;
	fbi->fb.flags		= FBINFO_FLAG_DEFAULT;
	fbi->fb.node		= -1;
	fbi->fb.monspecs	= monspecs;
	fbi->fb.disp		= (struct display *)(fbi + 1);
	fbi->fb.pseudo_palette	= (void *)(fbi->fb.disp + 1);

	fbi->rgb[RGB_8]		= &rgb_8;
	fbi->rgb[RGB_16]	= &def_rgb_16;

	inf = pl110fb_get_machine_info(fbi);

	fbi->max_xres				= inf->xres;
	fbi->fb.var.xres			= inf->xres;			/// 目前 lcd monitor 所使用的解析度, (但因為 LCD monitor 解析度通常不會變, 所以 ==> ..)
	fbi->fb.var.xres_virtual	= inf->xres;
	fbi->max_yres				= inf->yres;
	fbi->fb.var.yres			= inf->yres;
	fbi->fb.var.yres_virtual	= inf->yres;
	fbi->max_bpp				= inf->bpp;
	fbi->fb.var.bits_per_pixel	= inf->bpp;
	fbi->fb.var.pixclock		= inf->pixclock;
#ifdef not_complete_yet
	fbi->fb.var.hsync_len		= inf->hsync_len;
	fbi->fb.var.left_margin		= inf->left_margin;
	fbi->fb.var.right_margin	= inf->right_margin;
	fbi->fb.var.vsync_len		= inf->vsync_len;
	fbi->fb.var.upper_margin	= inf->upper_margin;
	fbi->fb.var.lower_margin	= inf->lower_margin;
#endif /* end_of_not */
	fbi->fb.var.sync			= inf->sync;
	fbi->fb.var.grayscale		= inf->cmap_greyscale;
	fbi->cmap_inverse			= inf->cmap_inverse;
	fbi->cmap_static			= inf->cmap_static;
	fbi->time0					= inf->time0;
	fbi->time1					= inf->time1;
	fbi->time2					= inf->time2;
	fbi->control				= inf->control;

	fbi->state					= C_DISABLE;				/// 目前 controller is disabled
	fbi->task_state				= (u_char)-1;
	//fbi->fb.fix.smem_len		= fbi->max_xres * fbi->max_yres * fbi->max_bpp / 8; // org
        fbi->fb.fix.smem_len            = 640 * 240 * fbi->max_bpp / 8;
	init_waitqueue_head(&fbi->ctrlr_wait);
	INIT_TQUEUE(&fbi->task, pl110fb_task, fbi);
	init_MUTEX(&fbi->ctrlr_sem);

	return fbi;
}

int __init pl110fb_init(void)
{
	struct pl110fb_info *fbi;
	int ret;

	fbi = pl110fb_init_fbinfo();
	ret = -ENOMEM;
	if (!fbi)
		goto failed;

	/* Initialize video memory */
	ret = pl110fb_map_video_memory(fbi);
	if (ret)
	{
		printk("memory allocate failure\n");
		goto failed;
	}
//ivan
//	ret = request_irq(FIQ_LCD, pl110fb_handle_irq, SA_INTERRUPT, fbi->fb.fix.id, fbi);
	if (ret) 
	{
		printk(KERN_ERR "pl110fb: request_irq failed: %d\n", ret);
		goto failed;
	}


	pl110fb_set_var(&fbi->fb.var, -1, &fbi->fb);			/// 設定 -1 這個 console 的 var

	ret = register_framebuffer(&fbi->fb);
	if (ret < 0)
		goto failed;


	/*
	 * Ok, now enable the LCD controller
	 */
	set_ctrlr_state(fbi, C_ENABLE);

	/* This driver cannot be unloaded at the moment */
	MOD_INC_USE_COUNT;

	return 0;

failed:
	if (fbi)
		kfree(fbi);
	return ret;
}

int __init pl110fb_setup(char *options)
{
	return 0;
}

void pl110fb_dummy()
{
	printk("enter pl110fb_dummy\n");
}


MODULE_DESCRIPTION("PL110fb framebuffer driver");
MODULE_LICENSE("GPL");
