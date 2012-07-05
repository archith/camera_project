/*
 *  Copyright (C) 2006-2008 Prolific Technology Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/mm.h>
#include <linux/init.h>

#include <asm/io.h>
#include <asm/mach-types.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <asm/mach/map.h>

/*
 * Chip specific mappings shared by all pl1097 systems
 */
static struct map_desc pl1097_sys_io_desc[] __initdata = {
    {rSYS_MANAGE_BASE,      rSYS_MANAGE_BASE_PHY,   rSYS_MANAGE_SIZE,   DOMAIN_IO, 0, 1 },
    {rCMC_BASE,             rCMC_BASE_PHY,          rCMC_SIZE,          DOMAIN_IO, 0, 1 },
    {rDMAC0_BASE,           rDMAC0_BASE_PHY,        rDMAC0_SIZE,        DOMAIN_IO, 0, 1 },
    {rPCI_IOPORT_BASE,      rPCI_IOPORT_BASE_PHY,   rPCI_IOPORT_SIZE,   DOMAIN_IO, 0, 1 },
};

static struct map_desc pl1097_peripherial_io_desc[] __initdata = {
    {rLCD_BASE,             rLCD_BASE_PHY,          rLCD_SIZE,          DOMAIN_IO, 0, 1 },
    {rUSB11_BASE,           rUSB11_BASE_PHY,        rUSB11_SIZE,        DOMAIN_IO, 0, 1 },
    {rUSB20_BASE,           rUSB20_BASE_PHY,        rUSB20_SIZE,        DOMAIN_IO, 0, 1 },
    {rAUDIO_OB_BASE,        rAUDIO_OB_BASE_PHY,     rAUDIO_OB_SIZE,     DOMAIN_IO, 0, 1 },
    {rAUDIO_IB_BASE,        rAUDIO_IB_BASE_PHY,     rAUDIO_IB_SIZE,     DOMAIN_IO, 0, 1 },
    {rAUDIO_CMD_BASE,       rAUDIO_CMD_BASE_PHY,    rAUDIO_CMD_SIZE,    DOMAIN_IO, 0, 1 },
    {rAUDIO_STATUS_BASE,    rAUDIO_STATUS_BASE_PHY, rAUDIO_STATUS_SIZE, DOMAIN_IO, 0, 1 },
    {rI2C_BASE,             rI2C_BASE_PHY,          rI2C_SIZE,          DOMAIN_IO, 0, 1 },
    {rIDE2_BASE,            rIDE2_BASE_PHY,         rIDE2_SIZE,         DOMAIN_IO, 0, 1 },
    {rNAND_BASE,            rNAND_BASE_PHY,         rNAND_SIZE,         DOMAIN_IO, 0, 1 },
    {rSD_BASE,              rSD_BASE_PHY,           rSD_SIZE,           DOMAIN_IO, 0, 1 },
    {rCF_BASE,              rCF_BASE_PHY,           rCF_SIZE,           DOMAIN_IO, 0, 1 },
    {rMSPRO_BASE,           rMSPRO_BASE_PHY,        rMSPRO_SIZE,        DOMAIN_IO, 0, 1 },
    {rUART_BASE,            rUART_BASE_PHY,         rUART_SIZE,         DOMAIN_IO, 0, 1 },
    {rMMX_LFB2PHB_BASE,     rMMX_LFB2PHB_BASE_PHY,  rMMX_LFB2PHB_SIZE,  DOMAIN_IO, 0, 1 },
    {rMMX_LMAPORT_BASE,     rMMX_LMAPORT_BASE_PHY,  rMMX_LMAPORT_SIZE,  DOMAIN_IO, 0, 1 },
    {rMMX_VPP_BASE,         rMMX_VPP_BASE_PHY,      rMMX_VPP_SIZE,      DOMAIN_IO, 0, 1 },
    {rMMX_PHP2LFB_BASE,     rMMX_PHP2LFB_BASE_PHY,  rMMX_PHP2LFB_SIZE,  DOMAIN_IO, 0, 1 },
    {rMMX_VDE_BASE,         rMMX_VDE_BASE_PHY,      rMMX_VDE_SIZE,      DOMAIN_IO, 0, 1 },
    LAST_DESC
};


void __init pl1097_map_io(void)
{
    iotable_init(pl1097_sys_io_desc);
    iotable_init(pl1097_peripherial_io_desc);
}