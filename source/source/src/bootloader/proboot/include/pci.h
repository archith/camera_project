#ifndef __PCI_H__
#define __PCI_H__

/*
 * PCI Configuration Space Header
 */
#define PCI_VENDOR_ID			0x00
#define PCI_DEVICE_ID			0x02
#define PCI_COMMAND			0x04
#define PCI_STATUS			0x06
#define PCI_CLASS_REVISION		0x08
#define PCI_CACHE_LINE_SIZE		0x0c
#define PCI_LATENCY_TIMER		0x0d
#define PCI_HEADER_TYPE			0x0e
#define PCI_BIST			0x0f
#define PCI_BASE_ADDRESS_0		0x10
#define PCI_BASE_ADDRESS_1		0x14
#define PCI_BASE_ADDRESS_2		0x18
#define PCI_BASE_ADDRESS_3		0x1c
#define PCI_BASE_ADDRESS_4		0x20
#define PCI_BASE_ADDRESS_5		0x24
#define PCI_CARDBUS_CIS			0x28
#define PCI_SUBSYSTEM_VENDOR_ID		0x2c
#define PCI_SUBSYSTEM_ID		0x2e  
#define PCI_ROM_ADDRESS			0x30
#define PCI_CAPABILITY_LIST		0x34
#define PCI_INTERRUPT_LINE		0x3c
#define PCI_INTERRUPT_PIN		0x3d
#define PCI_MIN_GNT			0x3e
#define PCI_MAX_LAT			0x3f

/*
 * Command Register
 */

#define	PCI_COMMAND_IO			0x1	/* Response in I/O space */
#define PCI_COMMAND_MEMORY		0x2	/* Response in Memory space */
#define PCI_COMMAND_MASTER		0x4	/* Enable bus mastering */

/*
 * Base Address Register
 */

#define PCI_BASE_ADDRESS_SPACE		0x01	/* 0 = memory, 1 = I/O */
#define PCI_BASE_ADDRESS_SPACE_IO	0x01
#define PCI_BASE_ADDRESS_SPACE_MEM	0x00
#define PCI_BASE_ADDRESS_MEM_MASK	(~0x0fUL)
#define PCI_BASE_ADDRESS_IO_MASK	(~0x03UL)


/*
 * PL106X Private
 */
#include <io.h>
#define PL_PCI_IO_BASE			KSEG1ADDR (0x1b800000)
#define PL_PCI_MEM_BASE			0x1a000000
#define PCI_CFG_ADDR			(PL_PCI_IO_BASE + 0xcf8)
#define PCI_CFG_DATA			(PL_PCI_IO_BASE + 0xcfc)

unsigned int pci_init_device (unsigned short vendor, unsigned short device);

#endif /* __PCI_H__ */
