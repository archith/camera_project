/*
 * drivers/usb/OTG/otg_whitelist.h
 *
 * Copyright (C) 2004 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#define CONFIG_USB_OTG_WHITELIST


/*
 * This OTG Whitelist is the OTG "Targeted Peripheral List".  It should
 * mostly use of USB_DEVICE() or USB_DEVICE_VER() entries..
 *
 * YOU _SHOULD_ CHANGE THIS LIST TO MATCH YOUR PRODUCT AND ITS TESTING!
 */ 

static struct usb_device_id whitelist_table [] = {

{ USB_DEVICE(0x2310, 0x5678) },//For Faraday OTG
{ USB_INTERFACE_INFO(0x08,0x06,0x50) },//For massstorage
{ USB_DEVICE(0x2310, 0x6688) },//For gadget file storage
{ USB_DEVICE(0x0000, 0x0000) },//For gadget file storage
#if	defined(CONFIG_USB_TEST) || defined(CONFIG_USB_TEST_MODULE)
/* gadget zero, for testing */
{ USB_DEVICE(0x0525, 0xa4a0) },
#endif

{ }	/* Terminating entry */
};



//*********************************************************
// Name: is_targeted
// Description:To find the target in the list
// Input: struct usb_device *dev
// Output:0 => no
//        1 => yes
//********************************************************* 
static int is_targeted(struct usb_device *dev)
{
        int iMatch=0;
	struct usb_device_id	*id = whitelist_table;

      printk(">>> Checking for 'is_targeted'...\n");

	/* possible in developer configs only! */
	if (!dev->bus->otg_port)
		{
		 printk("??? otg_whitelist.h-->is_targeted()-->return from 'if (!dev->bus->otg_port)'\n");
		 return 1;
		}

	/* HNP test device is _never_ targeted (see OTG spec 6.6.6) */
	if ((le16_to_cpu(dev->descriptor.idVendor) == 0x1a0a && 
	     le16_to_cpu(dev->descriptor.idProduct) == 0xbadd))
		{
		printk("??? test device is _never_ targeted'\n");
		return 0;
		}

	if ((le16_to_cpu(dev->descriptor.idVendor) == 0x1a0a && 
	     le16_to_cpu(dev->descriptor.idProduct) == 0x1234))
		{
		printk("??? Device Not Support...'\n");
		return 0;
		}




	/* NOTE: can't use usb_match_id() since interface caches
	 * aren't set up yet. this is cut/paste from that code.
	 */
	iMatch=0;
	for (id = whitelist_table; id->match_flags; id++) {
		
            //printk(">>> Checking for 'is_targeted'(id->match_flags=0x%x)...\n",id->match_flags);
		
	    //Checking for <1>.USB_DEVICE
              if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
                   (id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT))
                  {
                    if (id->idVendor == le16_to_cpu(dev->descriptor.idVendor))	
                  	if (id->idProduct == le16_to_cpu(dev->descriptor.idProduct))
                  	    {iMatch=1;
                  	     printk(">>> Device is in the target(USB_DEVICE=0x%x,0x%x)...\n",id->idVendor,id->idProduct);
                  	     return 1;//OK
                  	    }
                  } 
	    //Checking for <2>.USB_INTERFACE_INFO
#if 1	    
              if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_INFO))
                  {
                    if (id->bInterfaceClass == le16_to_cpu(dev->config->interface->altsetting->bInterfaceClass))	
                  	if (id->bInterfaceSubClass == le16_to_cpu(dev->config->interface->altsetting->bInterfaceSubClass))
                  	    if (id->bInterfaceProtocol == le16_to_cpu(dev->config->interface->altsetting->bInterfaceProtocol))
                  	       {iMatch=1;
                  	        printk(">>> **Device is in the target(USB_DEVICE_ID_MATCH_INT_INFO=0x%x,0x%x,0x%x)...\n"
                  	              ,id->bInterfaceClass,id->bInterfaceSubClass,id->bInterfaceProtocol);
                  	       return 1;//OK
                  	       }
                  } 
#endif
	}

	/* add other match criteria here ... */


        if (iMatch==1)
           {    //printk(">>> Device is in the target...\n");
		return 1;//OK
           }
        else
        {   
	/* OTG MESSAGE: report errors here, customize to match your product */
	printk("??? Device Not Support...(v%04x p%04x)\n",
		le16_to_cpu(dev->descriptor.idVendor),
		le16_to_cpu(dev->descriptor.idProduct));

         }

#ifdef	CONFIG_USB_OTG_WHITELIST
	return 0;
#else
	return 1;
#endif
}

