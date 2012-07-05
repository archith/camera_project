#include <linux/module.h>
#include <asm/io.h>		// readw, writew
#include <asm/arch/gio.h>	// name of base register

#define __read_gpio(gpio, base_reg, val)	\
	val = (readw(base_reg)>>gpio)&0x01;

#define __write_gpio(gpio, base_reg, val)	\
	{\
		unsigned short reg_val;\
		if(val)\
			reg_val = readw(base_reg) | (0x01<<gpio);\
		else\
			reg_val = readw(base_reg) & ~(0x01<<gpio);\
		writew(reg_val, base_reg);\
	}


void unrequest_gio(unsigned char gpio)
{
	if(gpio > 15)
		return;

	__write_gpio(gpio, GPIO_ENB, 0);
	__write_gpio(gpio, GPIO_OE, 0);
	__write_gpio(gpio, GPIO_KEY_ENB, 0);

	return;
}
//EXPORT_SYMBOL(unrequest_gio);

unsigned char gio_get_dir(unsigned char gpio)
{
	unsigned char in, out;
	if(gpio > 15)
		return -1;

	__read_gpio(gpio, GPIO_ENB, out);
	__read_gpio(gpio, GPIO_KEY_ENB, in);

	if(in && !out)
		return GIO_DIR_IN;
	else if(!in && out)
		return GIO_DIR_OUT;
	else // unknown
		return GIO_DIR_UNKNOWN;
}
//EXPORT_SYMBOL(gio_get_dir);

void gio_set_dir(unsigned char gpio, gio_dir_t dir)
{
	if(gpio > 15)
		return;

	if(dir == GIO_DIR_IN)
	{
		__write_gpio(gpio, GPIO_ENB, 0);
		__write_gpio(gpio, GPIO_KEY_ENB, 1);
	}
	else if(dir == GIO_DIR_OUT)
	{
		__write_gpio(gpio, GPIO_KEY_ENB, 0);
		__write_gpio(gpio, GPIO_ENB, 1);
	}

	return;
}
//EXPORT_SYMBOL(gio_set_dir);

unsigned char gio_get_oe(unsigned char gpio)
{
	unsigned char bit;
	if(gpio > 15)
		return -1;

	__read_gpio(gpio, GPIO_OE, bit);
	return bit;
}
//EXPORT_SYMBOL(gio_get_oe);

void gio_set_oe(unsigned char gpio, unsigned char bit)
{
	if(gpio > 15)
		return;

	__write_gpio(gpio, GPIO_OE, bit);
	return;
}
//EXPORT_SYMBOL(gio_set_oe);

unsigned char gio_get_inv(unsigned char gpio)
{
	unsigned char bit;
	if(gpio > 15)
		return -1;

	__read_gpio(gpio, GPIO_KEY_POL, bit);
	return bit;
}
//EXPORT_SYMBOL(gio_get_inv);

void gio_set_inv(unsigned char gpio, unsigned char bit)
{
	if(gpio > 15)
		return;

	__write_gpio(gpio, GPIO_KEY_POL, bit);
	return;
}
//EXPORT_SYMBOL(gio_set_inv);

unsigned char gio_get_input(unsigned char gpio)
{
	unsigned char val;
	if(gpio > 15)
		return -1;
	
	__read_gpio(gpio, GPIO_DI, val);
	return val;
}
//EXPORT_SYMBOL(gio_get_input);

unsigned char gio_get_output(unsigned char gpio)
{
	unsigned char val;
	if(gpio > 15)
		return -1;
	
	__read_gpio(gpio, GPIO_DO, val);
	return val;
}
//EXPORT_SYMBOL(gio_get_output);

void gio_set_output(unsigned char gpio, unsigned char bit)
{
	if(gpio > 15)
		return;

	__write_gpio(gpio, GPIO_DO, bit);
	return;
}
//EXPORT_SYMBOL(gio_set_output);

unsigned char gio_get_irq_mask(unsigned char gpio)
{
	unsigned char val;
	if(gpio > 15)
		return -1;
	
	__read_gpio(gpio, GPIO_KEY_MASK, val);
	return val;
}
//EXPORT_SYMBOL(gio_get_irq_mask);

void gio_set_irq_mask(unsigned char gpio, unsigned char bit)
{
	if(gpio > 15)
		return;

	__write_gpio(gpio, GPIO_KEY_MASK, bit);
	return;
}
//EXPORT_SYMBOL(gio_set_irq_mask);

unsigned char gio_get_irq_edge(unsigned char gpio)
{
	unsigned int val;
	if(gpio > 15)
		return -1;

	val = (readl(GPIO_KEY_EDGE) >> (gpio*2)) & 0x0011;
	return val;
}
//EXPORT_SYMBOL(gio_get_irq_edge);

void gio_set_irq_edge(unsigned char gpio, gio_edge_t edge)
{
	unsigned int val;
	if(gpio > 15)
		return;

	val = readl(GPIO_KEY_EDGE);

	if(edge == GIO_FALLING_EDGE)
	{
		val &= ~(1<<(gpio*2));
		val |= (1<<(gpio*2+1));
	}
	else if(edge == GIO_RISING_EDGE)
	{
		val |= (1<<(gpio*2));
		val &= ~(1<<(gpio*2+1));
	}
	else
	{
		val &= ~(1<<(gpio*2));
		val &= ~(1<<(gpio*2+1));
	}
	writel(val, GPIO_KEY_EDGE);

	return;
}
//EXPORT_SYMBOL(gio_set_irq_edge);

unsigned char gio_get_irq_level(unsigned char gpio)
{
	unsigned char val;
	if(gpio > 15)
		return -1;
	
	__read_gpio(gpio, GPIO_KEY_LEVEL, val);
	return val;
}
//EXPORT_SYMBOL(gio_get_irq_level);

void gio_set_irq_level(unsigned char gpio, unsigned char bit)
{
	if(gpio > 15)
		return;

	//__write_gpio(gpio, GPIO_KEY_LEVEL, bit);
	if(gpio < 8)
        {
                __write_gpio((gpio+8), (GPIO_KEY_LEVEL-0x01), bit);
        }
        else
        {
                __write_gpio((gpio-8), (GPIO_KEY_LEVEL+0x01), bit);
        }

	return;
}
//EXPORT_SYMBOL(gio_set_irq_level);

unsigned char gio_get_irq_status(unsigned char gpio)
{
	unsigned char val;
	if(gpio > 15)
		return -1;
	
	__read_gpio(gpio, GPIO_KEY_STATUS, val);
	return val;
}
//EXPORT_SYMBOL(gio_get_irq_status);

void gio_set_irq_status(unsigned char gpio, unsigned char bit)
{
	if(gpio > 15)
		return;

	__write_gpio(gpio, GPIO_KEY_STATUS, bit);
	return;
}
//EXPORT_SYMBOL(gio_set_irq_status);

/* Set gpio input function.
	flags: RSTN_INT_63, RSTN_SCAN_63, RSTN_KEY_63, RSTN_PWM_63
	** to setup gpio interrupt, use (RSTN_INT_63 | RSTN_KEY_63)
*/
void gio_set_in_func(unsigned char flags)
{
	writeb(readb(GPIO_RSTN_63)|flags, GPIO_RSTN_63);
	return;
}
//EXPORT_SYMBOL(gio_set_in_func);

