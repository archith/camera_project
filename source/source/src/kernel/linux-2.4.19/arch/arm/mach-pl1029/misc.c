#include <linux/module.h>
#include <linux/string.h>
//#include <asm/arch/hardware.h>
#include <asm/arch/misc.h>
#include <asm/arch/memconfig.h>
#include <asm/arch/memory.h>

#define MACADDR_LEN  	6          /* the same as if_ether.h (ETH_ALEN) */
#define MAC_PHY_ADDR  (BSPCONF_MAC6BYTE_OFFSET + BSPCONF_FLASH_BASE )
#define DOM_PHY_ADDR  (BSPCONF_WLAN_DOMAIN_OFFSET + BSPCONF_FLASH_BASE )
#define MAC_VIRT_ADDR   __phys_to_virt(MAC_PHY_ADDR)
#define DOM_VIRT_ADDR   __phys_to_virt(DOM_PHY_ADDR)
#define MACADDR(i)	*((volatile unsigned char *)(MAC_VIRT_ADDR + i))
#define DOMAIN(i)	*((volatile unsigned char *)(DOM_VIRT_ADDR + i))

int pl1029_check_addr(unsigned char *addr)
{
        int i;
        for (i=0; i<MACADDR_LEN; i++) { /* Check all <00> */
                if (addr[i] != 0x00) {
                        break;
                }
        }
        if (i == MACADDR_LEN) {
                return 0;       /* not valid */
        }
        for (i=0; i<MACADDR_LEN; i++) { /* Check all <FF> */
                if (addr[i] != 0xFF) {
                        break;
                }
        }
        if (i == MACADDR_LEN) {
                return 0;       /* not valid */
        }
        return 1;
}  // End of NC7XXMACAddrValid


void pl1029_get_macaddr(unsigned char *addr)
{
        unsigned char data[MACADDR_LEN];
	int i;

        /* ---- read from the flash ---- */
        memset(&data[0], '\0', MACADDR_LEN);
	for(i = 0; i< MACADDR_LEN; i++) {
		data[i] = MACADDR(i);
	}

        /* -- all <00>/<FF> ??, return default value. -- */
        /* -- but do NOT write back to the FLASH      -- */
        if (!pl1029_check_addr(&data[0])) {
                data[0] = 0x00;
                data[1] = 0xC0;
                data[2] = 0x02;
                data[3] = 0x00;
                data[4] = 0x00;
                data[5] = 0x00; /* default = 00:C0:02:00:00:00 */
        } /* end-if: if the data is invalid, use default */
        memcpy(addr, &data[0], MACADDR_LEN);
        return ;

}
void pl1029_get_wdomain(unsigned char *domain)
{
	*domain = DOMAIN(0);
	return ;
}
//EXPORT_SYMBOL(pl1029_get_macaddr);
