/*
 ***************************************************************************
 * Ralink Tech Inc.
 * 4F, No. 2 Technology 5th Rd.
 * Science-based Industrial Park
 * Hsin-chu, Taiwan, R.O.C.
 *
 * (c) Copyright 2002-2006, Ralink Technology, Inc.
 *
 * All rights reserved.	Ralink's source	code is	an unpublished work	and	the
 * use of a	copyright notice does not imply	otherwise. This	source code
 * contains	confidential trade secret material of Ralink Tech. Any attemp
 * or participation	in deciphering,	decoding, reverse engineering or in	any
 * way altering	the	source code	is stricitly prohibited, unless	the	prior
 * written consent of Ralink Technology, Inc. is obtained.
 ***************************************************************************

    Module Name:
    rtmp_main.c

    Abstract:

    Revision History:
    Who         When            What
    --------    ----------      ----------------------------------------------
    Name        Date            Modification logs
    Paul Lin    2002-11-25      Initial version
*/

#include "rt_config.h"
#ifdef _LED_
#include <asm/arch/gio.h>
#endif
//#include "sc_config.h"
// Global variable, debug level flag
#ifdef DBG
//ULONG   RTDebugLevel = RT_DEBUG_TRACE;
ULONG   RTDebugLevel = 0;
#endif

static dma_addr_t		dma_adapter;

extern	const struct iw_handler_def rt61_iw_handler_def;

#ifdef _TIWLAN_PROC_
static struct proc_dir_entry *  ra_proc_dir = NULL;
static int ra_drv_chr_read_proc(char *buf, char **start, off_t offset, int length, int *eof, void *data);
#endif
#ifdef _LED_
extern int TxRx;
struct  net_device      *net_dev_info;

/* 2006.11.24 add by Sercomm for Network (wireless) led blinking */
static struct timer_list  RT61LedTimer;
int TxRx= 0;

static void RT61LedSet(unsigned long ptr) {
	if(!(readw(GPIO_DO) & (1<<GIO_LED_2))) { 
		writew(readw(GPIO_DO) | (1<<GIO_LED_2),GPIO_DO); /* LED_ON */
		TxRx = 0;
	} else if(TxRx) {
		writew(readw(GPIO_DO) & ~(1<<GIO_LED_2),GPIO_DO); /* LED_OFF */
	}

	RT61LedTimer.expires =jiffies + (HZ >> 4);
	add_timer(&RT61LedTimer);
}
/* 2006.11.24 add by Sercomm for Network (wireless) led blinking */

#endif



// =======================================================================
// Ralink PCI device table, include all supported chipsets
// =======================================================================
static struct pci_device_id rt61_pci_tbl[] __devinitdata =
{
    {0x1814, 0x0301, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},		//RT2561S
   	{0x1814, 0x0302, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},		//RT2561 V2
    {0x1814, 0x0401, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0},		//RT2661
    {0,}		// terminate list
};
int const rt61_pci_tbl_len = sizeof(rt61_pci_tbl) / sizeof(struct pci_device_id);


/**************************
	Add for ethtool support. HAL/NetworkManager will use it.
***************************/
#define CSR_REG_BASE                    MAC_CSR0
#define CSR_REG_SIZE                    0x04b0
#define EEPROM_BASE                     0x0000
#define EEPROM_SIZE                     0x0100

static void
rt61_get_drvinfo(struct net_device *net_dev,
        struct ethtool_drvinfo *drvinfo)
{
        PRTMP_ADAPTER   pAd = net_dev->priv;

	printk("rt61_get_drvinfo\n");
#if 0
        strcpy(drvinfo->driver, NIC_DEVICE_NAME);
        strcpy(drvinfo->version, DRIVER_VERSION);
		
		snprintf(drvinfo->fw_version, sizeof(drvinfo->fw_version), "%d(%d)", pAd->FirmwareVersion, pAd->EepromVersion);
		usb_make_path(pAd->pUsb_Dev, drvinfo->bus_info, sizeof(drvinfo->bus_info));
#endif
}

static int
rt61_get_regs_len(struct net_device *net_dev)
{
	printk("rt61_get_regs_len\n");
#if 0
        return CSR_REG_SIZE;
#endif
}

static void
rt61_get_regs(struct net_device *net_dev,
        struct ethtool_regs *regs, void *data)
{
        PRTMP_ADAPTER   pAd = net_dev->priv;
        unsigned int counter;

	printk("rt61_get_regs\n");
#if 0
        regs->len = CSR_REG_SIZE;

        for (counter = 0; counter < CSR_REG_SIZE; counter += sizeof(UINT32)) {
                RTUSBReadMACRegister(pAd, CSR_REG_BASE + counter, (UINT32*)data);
                data += sizeof(UINT32);
        }
#endif
}


static UINT32 rt61_ethtool_get_link(struct net_device *dev)
{
	RTMP_ADAPTER *pAd;

	printk("rt61_ethtool_get_link\n");
#if 0
	ASSERT((dev));
	pAd = (PRTMP_ADAPTER) dev->priv;

	//We return true if we already associated to some AP.
	return(OPSTATUS_TEST_FLAG(pAd, fOP_STATUS_MEDIA_STATE_CONNECTED));
#endif
}

static int
rt61_get_eeprom_len(struct net_device *net_dev)
{
	printk("rt61_get_eeprom_len\n");
#if 0
        return EEPROM_SIZE;
#endif
}

static int
rt61_get_eeprom(struct net_device *net_dev,
        struct ethtool_eeprom *eeprom, u8 *data)
{
        PRTMP_ADAPTER   pAd = net_dev->priv;
        unsigned int counter;

	printk("rt61_get_eeprom\n");
#if 0
        for (counter = eeprom->offset; counter < eeprom->len; counter += sizeof(USHORT)) {
                USHORT value = 0;
				RTUSBReadEEPROM(pAd, CSR_REG_BASE + counter, value, sizeof(USHORT));
                memcpy(data, &value, sizeof(USHORT));
                data += sizeof(USHORT);
        }
#endif

        return 0;
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,19)
struct ethtool_ops rt61_ethtool_ops = {
#else
static const struct ethtool_ops rt61_ethtool_ops = {
#endif
        .get_drvinfo    = rt61_get_drvinfo,
        .get_regs_len   = rt61_get_regs_len,
        .get_regs       = rt61_get_regs,
        .get_link       = rt61_ethtool_get_link,
        .get_eeprom_len = rt61_get_eeprom_len,
        .get_eeprom     = rt61_get_eeprom,
};


static INT __devinit RT61_init_one (
    IN  struct pci_dev              *pPci_Dev,
    IN  const struct pci_device_id  *ent)
{
    INT rc;

    DBGPRINT(RT_DEBUG_TRACE, "===> RT61_init_one\n");

    // wake up and enable device
    if (pci_enable_device (pPci_Dev))
        rc = -EIO;
    else
        rc = RT61_probe(pPci_Dev, ent);

    DBGPRINT(RT_DEBUG_TRACE, "<=== RT61_init_one\n");
    return rc;
}

#if LINUX_VERSION_CODE <= 0x20402       // Red Hat 7.1
static struct net_device *alloc_netdev(int sizeof_priv, const char *mask, void (*setup)(struct net_device *))
{
    struct net_device	*dev;
    INT					alloc_size;

    /* ensure 32-byte alignment of the private area */
    alloc_size = sizeof (*dev) + sizeof_priv + 31;

    dev = (struct net_device *) kmalloc (alloc_size, MEM_ALLOC_FLAG);
    if (dev == NULL)
    {
        DBGPRINT(RT_DEBUG_ERROR, "alloc_netdev: Unable to allocate device memory.\n");
        return NULL;
    }

    memset(dev, 0, alloc_size);

    if (sizeof_priv)
        dev->priv = (void *) (((long)(dev + 1) + 31) & ~31);

    setup(dev);
    strcpy(dev->name,mask);

    return dev;
}
#endif

//
// PCI device probe & initialization function
//
INT __devinit   RT61_probe(
    IN  struct pci_dev              *pPci_Dev, 
    IN  const struct pci_device_id  *ent)
{
    struct  net_device      *net_dev;
    RTMP_ADAPTER            *pAd;
    CHAR                    *print_name;
    INT                     chip_id = (INT) ent->driver_data;
    unsigned long			csr_addr;
    INT                     Status;
	INT                     i;
	
    DBGPRINT(RT_DEBUG_TRACE, "Driver version-%s\n", DRIVER_VERSION);

    print_name = pPci_Dev ? pci_name(pPci_Dev) : "rt61";

#if LINUX_VERSION_CODE <= 0x20402       // Red Hat 7.1
    net_dev = alloc_netdev(sizeof(PRTMP_ADAPTER), "eth%d", ether_setup);
#else
    net_dev = alloc_etherdev(sizeof(PRTMP_ADAPTER));
#endif
    if (net_dev == NULL) 
    {
        DBGPRINT(RT_DEBUG_TRACE, "init_ethernet failed\n");
        goto err_out;
    }
#ifdef _TIWLAN_PROC_
    net_dev_info = net_dev;
#endif

    SET_MODULE_OWNER(net_dev);
        
    if (pci_request_regions(pPci_Dev, print_name))
        goto err_out_free_netdev;

    for (i = 0; i < rt61_pci_tbl_len; i++)
	{
		if (pPci_Dev->vendor == rt61_pci_tbl[i].vendor &&
			pPci_Dev->device == rt61_pci_tbl[i].device)
		{
			printk("RT61: Vendor = 0x%04x, Product = 0x%04x \n",pPci_Dev->vendor, pPci_Dev->device);
			break;
		}
	}
    if (i == rt61_pci_tbl_len) 
    {
		printk("Device PID/VID not matching!!!\n");
		goto err_out_free_netdev;
	}

    // Interrupt IRQ number
    net_dev->irq = pPci_Dev->irq;

    // map physical address to virtual address for accessing register
    csr_addr = (unsigned long) ioremap(pci_resource_start(pPci_Dev, 0), pci_resource_len(pPci_Dev, 0));
    if (!csr_addr) 
    {   
        DBGPRINT(RT_DEBUG_ERROR, "ioremap failed for device %s, region 0x%X @ 0x%lX\n",
            print_name, (ULONG)pci_resource_len(pPci_Dev, 0), pci_resource_start(pPci_Dev, 0));
        goto err_out_free_res;
    }

	net_dev->priv = pci_alloc_consistent(pPci_Dev, sizeof(RTMP_ADAPTER), &dma_adapter);

	// Zero init RTMP_ADAPTER
	memset(net_dev->priv, 0, sizeof(RTMP_ADAPTER));

    // Save CSR virtual address and irq to device structure
    net_dev->base_addr = csr_addr;
    pAd = net_dev->priv;
    pAd->CSRBaseAddress = (PVOID)net_dev->base_addr;
    pAd->net_dev = net_dev;

    // Set DMA master
    pci_set_master(pPci_Dev);

    pAd->chip_id = chip_id;
    pAd->pPci_Dev = pPci_Dev;

    // The chip-specific entries in the device structure.
    net_dev->open = RT61_open;
    net_dev->hard_start_xmit = RTMPSendPackets;
    net_dev->stop = RT61_close;
    net_dev->get_stats = RT61_get_ether_stats;

#if WIRELESS_EXT >= 12
    #if WIRELESS_EXT <= 20
    net_dev->get_wireless_stats = RT61_get_wireless_stats;
    #endif
    net_dev->wireless_handlers = (struct iw_handler_def *) &rt61_iw_handler_def;
#endif

    net_dev->set_multicast_list = RT61_set_rx_mode;
    net_dev->do_ioctl = RT61_ioctl;

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,5,0))
    SET_ETHTOOL_OPS(pAd->net_dev, &rt61_ethtool_ops);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0))
    SET_NETDEV_DEV(pAd->net_dev, &(pPci_Dev->dev));
#endif

    {// find available 
        INT     i=0;
        CHAR    slot_name[IFNAMSIZ];
        struct net_device   *device;

        for (i = 0; i < 8; i++)
        {
#if 0
            sprintf(slot_name, "ra%d", i);
#else
            sprintf(slot_name, "eth%d", i);
#endif
            
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,0)            
	if(dev_get_by_name(slot_name)==NULL)                
		break;
#else     
            read_lock_bh(&dev_base_lock); // avoid multiple init
            for (device = dev_base; device != NULL; device = device->next)
            {
                if (strncmp(device->name, slot_name, 4) == 0)
                {
                    break;
                }
            }
            read_unlock_bh(&dev_base_lock);
#endif
            if(device == NULL)
                break;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
            else
                dev_put(device);
#endif
        }
        if(i == 8)
        {
            DBGPRINT(RT_DEBUG_ERROR, "No available slot name\n");
            goto err_out_unmap;
        }

#if 0
        sprintf(net_dev->name, "ra%d", i);
#else
        sprintf(net_dev->name, "eth%d", i);
#endif
    }

    // Register this device
    Status = register_netdev(net_dev);
    if (Status)
        goto err_out_unmap;

    DBGPRINT(RT_DEBUG_TRACE, "%s: at 0x%lx, VA 0x%1x, IRQ %d. \n", 
        net_dev->name, pci_resource_start(pPci_Dev, 0), (ULONG)csr_addr, pPci_Dev->irq);

    // Set driver data
    pci_set_drvdata(pPci_Dev, net_dev);

#ifdef _TIWLAN_PROC_
    ra_proc_dir = create_proc_entry("tiwlan", S_IFDIR, &proc_root);
    if(ra_proc_dir == NULL) {
	printk("create_proc_entry failed\n");
    }

    create_proc_read_entry("status",0,ra_proc_dir,ra_drv_chr_read_proc,NULL);
#endif
#ifdef _LED_
    /* 2006.11.20 add by Sercomm for wireless led blinking */
    init_timer(&RT61LedTimer);
    RT61LedTimer.function = RT61LedSet;
    RT61LedTimer.expires =  jiffies + 10;
    add_timer(&RT61LedTimer);
    /* 2006.11.20 add by Sercomm for wireless led blinking */
#endif
#ifdef WSC_SUPPORTxxx
    init_MUTEX_LOCKED(&(pAd->write_dat_file_semaphore));
    init_completion(&pAd->write_dat_file_notify);
    start_write_dat_file_thread(pAd);
#endif // WSC_SUPPORT //
    

    return 0;

err_out_unmap:
    iounmap((void *)csr_addr);
    release_mem_region(pci_resource_start(pPci_Dev, 0), pci_resource_len(pPci_Dev, 0));
err_out_free_res:
    pci_release_regions(pPci_Dev);
    
err_out_free_netdev:
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    kfree(net_dev);
#else
    free_netdev(net_dev);
#endif

err_out:
    return -ENODEV;
}

INT RT61_open(
    IN  struct net_device *net_dev)
{
    PRTMP_ADAPTER   pAd = net_dev->priv;
    INT             status = NDIS_STATUS_SUCCESS;
    ULONG		    MacCsr0;
    UCHAR           		TmpPhy;
	
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    if (!try_module_get(THIS_MODULE))
    {
        DBGPRINT(RT_DEBUG_ERROR, "%s: cannot reserve module\n", __FUNCTION__);
        return -1;
    }
#else
    MOD_INC_USE_COUNT;
#endif

    // Wait for hardware stable
	{
		ULONG MacCsr0, Index = 0;
		
		do
		{
			RTMP_IO_READ32(pAd, MAC_CSR0, &MacCsr0);

			if (MacCsr0 != 0)
				break;
			
			RTMPusecDelay(1000);
		} while (Index++ < 1000);
	}
	
	RTMPAllocAdapterBlock(pAd);
			
    // 1. Allocate DMA descriptors & buffers
    status = RTMPAllocDMAMemory(pAd);
    if (status != NDIS_STATUS_SUCCESS)
        goto out_module_put;

    //
	// 1.1 Init TX/RX data structures and related parameters
	//
	NICInitTxRxRingAndBacklogQueue(pAd);

    // 2. request interrupt
    // Disable interrupts here which is as soon as possible
    // This statement should never be true. We might consider to remove it later
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_ACTIVE))
    {
        NICDisableInterrupt(pAd);
    }

    status = request_irq(pAd->pPci_Dev->irq, &RTMPIsr, SA_SHIRQ, net_dev->name, net_dev);
    if (status)
    {
        goto out_free_dma_memory;
    }
    RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE);

    
    // Initialize pAd->UserCfg to manufacture default
    //
	PortCfgInit(pAd);

	// Load 8051 firmware
    status = NICLoadFirmware(pAd);
    if(status != NDIS_STATUS_SUCCESS)
    {
        goto out_mlme_halt;
    }
    

    // Initialize Asics
    NICInitializeAdapter(pAd);
	
#ifdef RT_READ_PROFILE
    // Read RaConfig profile parameters  
    RTMPReadParametersFromFile(pAd);
#endif
    // We should read EEPROM for all cases.
	NICReadEEPROMParameters(pAd);
	

	// hardware initialization after all parameters are acquired from
    // Registry or E2PROM
	TmpPhy = pAd->PortCfg.PhyMode;
	pAd->PortCfg.PhyMode = 0xff;
	RTMPSetPhyMode(pAd, TmpPhy);

    NICInitAsicFromEEPROM(pAd);


    //
    // other settings
    //
    
    // PCI_ID info
    pAd->VendorDesc = (ULONG)(pAd->pPci_Dev->vendor << 16) + (pAd->pPci_Dev->device);
    
	// Note: DeviceID = 0x0302 shoud turn off the Aggregation.
	if (pAd->pPci_Dev->device == NIC2561_PCI_DEVICE_ID)
		pAd->PortCfg.bAggregationCapable = FALSE;
	
	RTMP_IO_READ32(pAd, MAC_CSR0, &MacCsr0);
	if (((MacCsr0 == 0x2561c) || (MacCsr0 == 0x2661d)) && (pAd->PortCfg.bAggregationCapable == TRUE))
	{
		pAd->PortCfg.bPiggyBackCapable = TRUE;
	}
	else
	{
		pAd->PortCfg.bPiggyBackCapable = FALSE;
	}

    // external LNA For 5G has different R17 base
    if (pAd->NicConfig2.field.ExternalLNAForA)
    {
	    pAd->BbpTuning.R17LowerBoundA += 0x10;
	    pAd->BbpTuning.R17UpperBoundA += 0x10;
    }

    // external LNA For 2.4G has different R17 base
    if (pAd->NicConfig2.field.ExternalLNAForG)
    {
        pAd->BbpTuning.R17LowerBoundG += 0x10;
        pAd->BbpTuning.R17UpperBoundG += 0x10;
    }	


    net_dev->dev_addr[0] = pAd->CurrentAddress[0];
	net_dev->dev_addr[1] = pAd->CurrentAddress[1];
	net_dev->dev_addr[2] = pAd->CurrentAddress[2];
	net_dev->dev_addr[3] = pAd->CurrentAddress[3];
	net_dev->dev_addr[4] = pAd->CurrentAddress[4];
	net_dev->dev_addr[5] = pAd->CurrentAddress[5];


    // initialize MLME
    status = MlmeInit(pAd);
    if(status != NDIS_STATUS_SUCCESS)
    {
        goto out_free_irq;
    }
    
#ifdef THREAD_ISR
	RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_REMOVE_IN_PROGRESS);
	Rtmp_Init_Thread_Task(pAd);
#endif
    
    //
	// Enable Interrupt
	//
	RTMP_IO_WRITE32(pAd, INT_SOURCE_CSR, 0xffffffff);  // clear garbage interrupts
    NICEnableInterrupt(pAd, 0);

    //
    // Now Enable Rx
    //
    RTMP_IO_WRITE32(pAd, RX_CNTL_CSR, 0x00000001);  // enable RX of DMA block
    RTMP_IO_WRITE32(pAd, TXRX_CSR0, 0x025eb032);    // enable RX of MAC block, Staion not drop control frame

    RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_START_UP);

    // Start net interface tx /rx
    netif_start_queue(net_dev);

    netif_carrier_on(net_dev);
    netif_wake_queue(net_dev);

    return 0;

out_mlme_halt:
	MlmeHalt(pAd);
out_free_irq:
	free_irq(net_dev->irq, net_dev);
out_free_dma_memory:
	RTMPFreeDMAMemory(pAd);	
out_module_put:
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    module_put(THIS_MODULE);
#else
    MOD_DEC_USE_COUNT;
#endif

    return status;
}

/*
    ========================================================================

    Routine Description:
        hard_start_xmit handler

    Arguments:
        skb             point to sk_buf which upper layer transmit
        net_dev         point to net_dev
    Return Value:
        None

    Note:

    ========================================================================
*/
INT RTMPSendPackets(
    IN  struct sk_buff		*pSkb,
    IN  struct net_device	*net_dev)
{
    UCHAR Index;
    PRTMP_ADAPTER   pAdapter = net_dev->priv;

    DBGPRINT(RT_DEBUG_INFO, "===> RTMPSendPackets\n");

#ifdef RALINK_ATE
	if (pAdapter->ate.Mode != ATE_STASTART)
	{
		RTMPFreeSkbBuffer(pSkb);
		return 0;
	}
#endif

#ifdef _LED_
    TxRx++;
#endif

    // Drop packets if no associations
    if (!INFRA_ON(pAdapter) && !ADHOC_ON(pAdapter))
    {
        // Drop send request since there are no physical connection yet
        // Check the association status for infrastructure mode
        // And Mibss for Ad-hoc mode setup
        RTMPFreeSkbBuffer(pSkb);
    }
    else if (RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		     RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_HALT_IN_PROGRESS))
	{
		// Drop send request since hardware is in reset state
		RTMPFreeSkbBuffer(pSkb);
	}
    else
    {
        // initial pSkb->data_len=0, we will use this variable to store data size when fragment(in TKIP)
        // and pSkb->len is actual data len
        pSkb->data_len = pSkb->len;

        // Record that orignal packet source is from protocol layer,so that 
		// later on driver knows how to release this skb buffer
		RTMP_SET_PACKET_SOURCE(pSkb, PKTSRC_NDIS);
        pAdapter->RalinkCounters.PendingNdisPacketCount ++;
        RTMPSendPacket(pAdapter, pSkb);
    }

    // Dequeue one frame from TxSwQueue[] and process it
//    if ((!RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_BSS_SCAN_IN_PROGRESS)) && 
//		(!RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_RESET_IN_PROGRESS)) &&
//		(!RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_HALT_IN_PROGRESS)))
    {
        // let the subroutine select the right priority Tx software queue
        for(Index=0;Index <5; Index++)
        {
		if(pAdapter->TxSwQueue[Index].Number >0)
		  RTMPDeQueuePacket(pAdapter, Index);
	}	
    }

    return 0;
}

/*
    ========================================================================

    Routine Description:
        Interrupt handler

    Arguments:
        irq                         interrupt line
        dev_instance                Pointer to net_device
        rgs                         store process's context before entering ISR, 
                                    this parameter is just for debug purpose.

    Return Value:
        VOID

    Note:

    ========================================================================
*/
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
irqreturn_t
#else
VOID
#endif
RTMPIsr(
    IN  INT             irq, 
    IN  VOID            *dev_instance, 
    IN  struct pt_regs  *rgs)
{
    struct net_device		*net_dev = dev_instance;
    PRTMP_ADAPTER			pAdapter = net_dev->priv;
    INT_SOURCE_CSR_STRUC	IntSource;
    MCU_INT_SOURCE_STRUC	McuIntSource;
    UINT32					int_mask = 0;

    DBGPRINT(RT_DEBUG_INFO, "====> RTMPHandleInterrupt\n");
    //
	// Inital the Interrupt source.
	//
	IntSource.word = 0x00000000L;
	McuIntSource.word = 0x00000000L;

    // 1. Disable interrupt
	if (RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_INTERRUPT_IN_USE) && RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_INTERRUPT_ACTIVE))
	{
		NICDisableInterrupt(pAdapter);
	}

    //
	// Get the interrupt sources & saved to local variable
	//
    RTMP_IO_READ32(pAdapter, MCU_INT_SOURCE_CSR, &McuIntSource.word);
    RTMP_IO_WRITE32(pAdapter, MCU_INT_SOURCE_CSR, McuIntSource.word);
	RTMP_IO_READ32(pAdapter, INT_SOURCE_CSR, &IntSource.word);
	RTMP_IO_WRITE32(pAdapter, INT_SOURCE_CSR, IntSource.word); // write 1 to clear

    //
	// Handle interrupt, walk through all bits
	// Should start from highest priority interrupt
	// The priority can be adjust by altering processing if statement
	//

	// If required spinlock, each interrupt service routine has to acquire
	// and release itself.
	//

#ifndef THREAD_ISR
    if (IntSource.field.MgmtDmaDone)
        RTMPHandleMgmtRingDmaDoneInterrupt(pAdapter);
    
	if (IntSource.field.RxDone)
		RTMPHandleRxDoneInterrupt(pAdapter);

	if (IntSource.field.TxDone)
		RTMPHandleTxDoneInterrupt(pAdapter);

    if (IntSource.word & 0x002f0000)
        RTMPHandleTxRingDmaDoneInterrupt(pAdapter, IntSource);
#else

	int_mask = Rtmp_Thread_Isr(pAdapter, IntSource);

#endif /* THREAD_ISR */

    if (McuIntSource.word & 0xff)
    {
        ULONG M2hCmdDoneCsr;
        RTMP_IO_READ32(pAdapter, M2H_CMD_DONE_CSR, &M2hCmdDoneCsr);
        RTMP_IO_WRITE32(pAdapter, M2H_CMD_DONE_CSR, 0xffffffff);
        DBGPRINT(RT_DEBUG_TRACE,"MCU command done - INT bitmap=0x%02x, M2H mbox=0x%08x\n", McuIntSource.word, M2hCmdDoneCsr);
    }

    if (McuIntSource.field.TBTTExpire)
		RTMPHandleTBTTInterrupt(pAdapter);
		
    if (McuIntSource.field.Twakeup)
        RTMPHandleTwakeupInterrupt(pAdapter);

    // Do nothing if Reset in progress
    if (RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_RESET_IN_PROGRESS) ||
		RTMP_TEST_FLAG(pAdapter, fRTMP_ADAPTER_HALT_IN_PROGRESS))
    {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
        return  IRQ_HANDLED;
#else
        return;
#endif
    }

    //
    // Re-enable the interrupt (disabled in RTMPIsr)
    //
    NICEnableInterrupt(pAdapter, int_mask);

    DBGPRINT(RT_DEBUG_INFO, "<==== RTMPHandleInterrupt\n");
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    return  IRQ_HANDLED;
#endif
}

#if (WIRELESS_EXT >= 12)
/*
    ========================================================================

    Routine Description:
        get wireless statistics

    Arguments:
        net_dev                     Pointer to net_device

    Return Value:
        struct iw_statistics

    Note:
        This function will be called when query /proc

    ========================================================================
*/
long rt_abs(long arg)	{	return (arg<0)? -arg : arg;}
struct iw_statistics *RT61_get_wireless_stats(
    IN  struct net_device *net_dev)
{
    RTMP_ADAPTER *pAd = net_dev->priv;

    DBGPRINT(RT_DEBUG_TRACE, "RT61_get_wireless_stats --->\n");

    // TODO: All elements are zero before be implemented

    pAd->iw_stats.status = 0;   // Status - device dependent for now

    pAd->iw_stats.qual.qual = pAd->Mlme.ChannelQuality; // link quality (%retries, SNR, %missed beacons or better...)
#ifdef RTMP_EMBEDDED
    pAd->iw_stats.qual.level = rt_abs(pAd->PortCfg.LastRssi);   // signal level (dBm)
#else
    pAd->iw_stats.qual.level = abs(pAd->PortCfg.LastRssi);      // signal level (dBm)
#endif
	pAd->iw_stats.qual.level += 256 - pAd->BbpRssiToDbmDelta;
        
    pAd->iw_stats.qual.noise = (pAd->BbpWriteLatch[17] > pAd->BbpTuning.R17UpperBoundG) ? pAd->BbpTuning.R17UpperBoundG : ((ULONG) pAd->BbpWriteLatch[17]);     // noise level (dBm)
    pAd->iw_stats.qual.noise += 256 - 143;
    pAd->iw_stats.qual.updated = 1;     // Flags to know if updated

    pAd->iw_stats.discard.nwid = 0;     // Rx : Wrong nwid/essid
    pAd->iw_stats.miss.beacon = 0;      // Missed beacons/superframe

    // pAd->iw_stats.discard.code, discard.fragment, discard.retries, discard.misc has counted in other place

    return &pAd->iw_stats;
}
#endif

/*
    ========================================================================

    Routine Description:
        return ethernet statistics counter

    Arguments:
        net_dev                     Pointer to net_device

    Return Value:
        net_device_stats*

    Note:

    ========================================================================
*/
struct net_device_stats *RT61_get_ether_stats(
    IN  struct net_device *net_dev)
{
    RTMP_ADAPTER *pAd = net_dev->priv;

    DBGPRINT(RT_DEBUG_INFO, "RT61_get_ether_stats --->\n");
 
    pAd->stats.rx_packets = pAd->WlanCounters.ReceivedFragmentCount.vv.LowPart;        // total packets received
    pAd->stats.tx_packets = pAd->WlanCounters.TransmittedFragmentCount.vv.LowPart;     // total packets transmitted

    pAd->stats.rx_bytes= pAd->RalinkCounters.ReceivedByteCount;             // total bytes received
    pAd->stats.tx_bytes = pAd->RalinkCounters.TransmittedByteCount;         // total bytes transmitted

    pAd->stats.rx_errors = pAd->Counters8023.RxErrors;                      // bad packets received
    pAd->stats.tx_errors = pAd->Counters8023.TxErrors;                      // packet transmit problems

    pAd->stats.rx_dropped = pAd->Counters8023.RxNoBuffer;                   // no space in linux buffers
    pAd->stats.tx_dropped = pAd->WlanCounters.FailedCount.vv.LowPart;                  // no space available in linux

    pAd->stats.multicast = pAd->WlanCounters.MulticastReceivedFrameCount.vv.LowPart;   // multicast packets received
    pAd->stats.collisions = pAd->Counters8023.OneCollision + pAd->Counters8023.MoreCollisions;  // Collision packets

    pAd->stats.rx_length_errors = 0;
    pAd->stats.rx_over_errors = pAd->Counters8023.RxNoBuffer;               // receiver ring buff overflow
    pAd->stats.rx_crc_errors = 0;//pAd->WlanCounters.FCSErrorCount;         // recved pkt with crc error
    pAd->stats.rx_frame_errors = pAd->Counters8023.RcvAlignmentErrors;      // recv'd frame alignment error
    pAd->stats.rx_fifo_errors = pAd->Counters8023.RxNoBuffer;               // recv'r fifo overrun
    pAd->stats.rx_missed_errors = 0;                                        // receiver missed packet

    // detailed tx_errors
    pAd->stats.tx_aborted_errors = 0;
    pAd->stats.tx_carrier_errors = 0;
    pAd->stats.tx_fifo_errors = 0;
    pAd->stats.tx_heartbeat_errors = 0;
    pAd->stats.tx_window_errors = 0;

    // for cslip etc
    pAd->stats.rx_compressed = 0;
    pAd->stats.tx_compressed = 0;
       
    return &pAd->stats;
}

/*
    ========================================================================

    Routine Description:
        Set to filter multicast list

    Arguments:
        net_dev                     Pointer to net_device

    Return Value:
        VOID

    Note:

    ========================================================================
*/
VOID RT61_set_rx_mode(
    IN  struct net_device *net_dev)
{
    // RTMP_ADAPTER *pAd = net_dev->priv;
    // TODO: set_multicast_list
}

#ifdef _TIWLAN_PROC_
static int ra_drv_chr_read_proc(char *buf, char **start, off_t offset, int length, int *eof, void *data) {
    	RTMP_ADAPTER        *pAdapter = net_dev_info->priv; 
	char		    essid[IW_ESSID_MAX_SIZE+1];			
	int len=0;
	SHORT   	    dbm;

        if (OPSTATUS_TEST_FLAG(pAdapter, fOP_STATUS_MEDIA_STATE_CONNECTED)) {
		// SSID
		memset(essid,0,sizeof(essid));
		memcpy(essid, &pAdapter->PortCfg.Ssid, pAdapter->PortCfg.SsidLen);
		len = sprintf(buf, "ssid %s\n", essid);
		// BSSID
		len += sprintf(buf+len, "bssid %02x:%02x:%02x:%02x:%02x:%02x\n", pAdapter->PortCfg.Bssid[0],pAdapter->PortCfg.Bssid[1],pAdapter->PortCfg.Bssid[2],pAdapter->PortCfg.Bssid[3],pAdapter->PortCfg.Bssid[4],pAdapter->PortCfg.Bssid[5]);

		// Channel
		len += sprintf(buf+len, "channel %d\n", pAdapter->PortCfg.Channel);
		// Strength
		dbm = pAdapter->PortCfg.AvgRssi - pAdapter->BbpRssiToDbmDelta;
		if ((pAdapter->RfIcType == RFIC_5325) || (pAdapter->RfIcType == RFIC_2529))
		{
			if (pAdapter->PortCfg.AvgRssi2 > pAdapter->PortCfg.AvgRssi)
			{
				dbm = pAdapter->PortCfg.AvgRssi2 - pAdapter->BbpRssiToDbmDelta;
			}
		}

		len += sprintf(buf+len, "strength %d\n", dbm);
		// speed
		len += sprintf(buf+len, "speed %dMb/s\n", RateIdTo500Kbps[pAdapter->PortCfg.TxRate] * 500000 / 1000000);

	 } 

	return len;
}
#endif
//
// Close driver function
//
INT RT61_close(
    IN  struct net_device *net_dev)
{
    RTMP_ADAPTER    *pAd = net_dev->priv;
    // LONG            ioaddr = net_dev->base_addr;
    BOOLEAN Cancelled;
    DBGPRINT(RT_DEBUG_TRACE, "===> RT61_close\n");

#ifdef THREAD_ISR
	RTMP_SET_FLAG(pAd, fRTMP_ADAPTER_REMOVE_IN_PROGRESS);
	Rtmp_Kill_Thread_Task(pAd);
#endif /* TASK_LEVL_ISR */

    RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_START_UP);


    // Stop Mlme state machine
    RTMPCancelTimer(&pAd->RfTuningTimer, &Cancelled);
    MlmeHalt(pAd);

#ifdef MAT_SUPPORT  //Add by Zero:Jul.04.2007
    MATEngineExit();
#endif // MAT_SUPPORT //

    netif_stop_queue(net_dev);
    netif_carrier_off(net_dev);

    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_ACTIVE))
    {
        NICDisableInterrupt(pAd);
    }

    // Disable Rx, register value supposed will remain after reset
    NICIssueReset(pAd);

    // Free IRQ
    if (RTMP_TEST_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE))
    {
        // Deregister interrupt function
        free_irq(net_dev->irq, net_dev);
        RTMP_CLEAR_FLAG(pAd, fRTMP_ADAPTER_INTERRUPT_IN_USE);
    }

    // Free Ring buffers
    RTMPFreeDMAMemory(pAd);

    
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
    module_put(THIS_MODULE);
#else
    MOD_DEC_USE_COUNT;
#endif

    return 0;
}

//
// Remove driver function
//
static VOID __devexit RT61_remove_one(
    IN  struct pci_dev  *pPci_Dev)
{
    struct net_device   *net_dev = pci_get_drvdata(pPci_Dev);
    RTMP_ADAPTER        *pAd = net_dev->priv; 

    DBGPRINT(RT_DEBUG_TRACE, "===> RT61_remove_one\n");
    

    // Unregister network device
    unregister_netdev(net_dev);

    // Unmap CSR base address
    iounmap((char *)(net_dev->base_addr));

    pci_free_consistent(pAd->pPci_Dev, sizeof(RTMP_ADAPTER), pAd, dma_adapter);

    // release memory region
    release_mem_region(pci_resource_start(pPci_Dev, 0), pci_resource_len(pPci_Dev, 0));

#ifdef _TIWLAN_PROC_
    remove_proc_entry("status", ra_proc_dir);
    remove_proc_entry("tiwlan", NULL);
#endif
    // Free pre-allocated net_device memory
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,5,0)
    kfree(net_dev);
#else
    free_netdev(net_dev);
#endif    
}

// =======================================================================
// Our PCI driver structure
// =======================================================================
static struct pci_driver rt61_driver =
{
    name:       "rt61",
    id_table:   rt61_pci_tbl,
    probe:      RT61_init_one,
#if LINUX_VERSION_CODE >= 0x20412 || BIG_ENDIAN == TRUE || RTMP_EMBEDDED == TRUE
    remove:     __devexit_p(RT61_remove_one),
#else
    remove:     __devexit(RT61_remove_one),
#endif
};

// =======================================================================
// LOAD / UNLOAD sections
// =======================================================================
//
// Driver module load function
//
static INT __init rt61_init_module(VOID)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,5,0)
	return pci_register_driver(&rt61_driver);
#else
    return pci_module_init(&rt61_driver);
#endif
}

//
// Driver module unload function
//
static VOID __exit rt61_cleanup_module(VOID)
{
#ifdef _LED_
    del_timer_sync(&RT61LedTimer);
#endif

    pci_unregister_driver(&rt61_driver);
}

/*************************************************************************/
// Following information will be show when you run 'modinfo'
// *** If you have a solution for the bug in current version of driver, please mail to me.
// Otherwise post to forum in ralinktech's web site(www.ralinktech.com) and let all users help you. ***

MODULE_AUTHOR("Paul Lin <paul_lin@ralinktech.com>");
MODULE_DESCRIPTION("RT61 Wireless Lan Linux Driver");

// *** open source release
//MODULE_LICENSE("GPL");

MODULE_DEVICE_TABLE(pci, rt61_pci_tbl);

module_init(rt61_init_module);
module_exit(rt61_cleanup_module);
/*************************************************************************/

