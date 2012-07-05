#ifndef _ZD_INLINEF_H
#define _ZD_INLINEF_H
#include "zd1205.h"

extern zd_80211Obj_t dot11Obj;

__inline void cal_us_service(u8 tx_rate, u16 len,	u16	*len_in_us,	u8 *service)
{
	u32		remainder;
	int		delta;

	*(service) = 0x04;
	switch(tx_rate){
		case RATE_1M:		/* 1M bps */
			*(len_in_us) = len << 3;
			break;
			
		case RATE_2M:		/* 2M bps */
			*(len_in_us) = len << 2;
			break;
			
		case RATE_5M:		/* 5.5M bps */
			*(len_in_us) = (u16)(((u32)len << 4)/11);
			remainder = (((u32)len << 4) % 11);
			if ( remainder ){
				*(len_in_us) += 1;
			}
			break;
			
		case RATE_11M:		/* 11M bps */
			*(len_in_us) = (u16)(((u32)len << 3)/11);
			remainder = (((u32)len << 3) % 11);
			delta = 11 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta >= 8 ){
					*(service) |= BIT_7;
				}
			}
			break;
		
		case RATE_16M:		// 16.5M bps 
			*(len_in_us) = (u16)(((u32)len << 4)/33);
			remainder = (((u32)len << 4) % 33);
			delta = 33 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 16 ){
				}
				else if ( (delta >= 16) && (delta < 32) ){
					*(service) |= BIT_7;
				}
				else if ( delta >= 32 ){
					*(service) |= BIT_6;
				}
			}
			break;
				
		case RATE_22M:		// 22M bps 
			*(len_in_us) = (u16)(((u32)len << 2)/11);
			remainder = (((u32)len << 2) % 11);
			delta = 11 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 4 ){
				}
				else if ( (delta >= 4) && (delta < 8) ){
					*(service) |= BIT_7;
				}
				else if ( delta >= 8 ){
					*(service) |= BIT_6;
				}
			}
			break;
			
		
			
		case RATE_27M:		// 27.5 bps 
			*(len_in_us) = (u16)(((u32)len << 4)/55);
			remainder = (((u32)len << 4) % 55);
			delta = 55 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 16 ){
				}
				else if ( (delta >= 16) && (delta < 32) ){
					*(service) |= BIT_7;
				}

				else if ( (delta >= 32) && (delta < 48) ){
					*(service) |= BIT_6;
				}
				else if ( delta >= 48 ){
					*(service) |= (BIT_6 | BIT_7);
				}
			}
			break;
			
		case 7:		// 33M bps
			*(len_in_us) = (u16)(((u32)len << 3)/33);
			remainder = (((u32)len << 3) % 33);
			delta = 33 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 8 ){
				}
				else if ( (delta >= 8) && (delta < 16) ){
					*(service) |= BIT_7;
				}
				else if ( (delta >= 16) && (delta < 24) ){
					*(service) |= BIT_6;
				}
				else if ( (delta >= 24) && (delta < 32) ){
					*(service) |= (BIT_6 | BIT_7);
				}
				else if ( delta >= 32 ){
					*(service) |= BIT_5;
				}
			}
			break;
			
		case 8:		// 38.5M bps
			*(len_in_us) = (u16)(((u32)len << 4)/77);
			remainder = (((u32)len << 4) % 77);
			delta = 77 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 16 ){
				}
				else if ( (delta >= 16) && (delta < 32) ){
					*(service) |= BIT_7;


				}
				else if ( (delta >= 32) && (delta < 48) ){
					*(service) |= BIT_6;
				}
				else if ( (delta >= 48) && (delta < 64) ){
					*(service) |= (BIT_6 | BIT_7);
				}
				else if ( delta >= 64){
					*(service) |= BIT_5;
				}
			}
			break;
			
		case 9:		// 44M bps
			*(len_in_us) = (u16)(((u32)len << 1)/11);
			remainder = (((u32)len << 1) % 11);
			delta = 11 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 2 ){
				}
				else if ( (delta >= 2) && (delta < 4) ){
					*(service) |= BIT_7;
				}
				else if ( (delta >= 4) && (delta < 6) ){
					*(service) |= BIT_6;
				}
				else if ( (delta >= 6) && (delta < 8) ){
					*(service) |= (BIT_6 | BIT_7);
				}
				else if ( (delta >= 8) && (delta < 10) ){
					*(service) |= BIT_5;
				}
				else if ( delta >= 10 ){
					*(service) |= (BIT_5 | BIT_7);
				}
			}
			break;
			
		case 10:		// 49.5M bps
			*(len_in_us) = (u16)(((u32)len << 4)/99);
			remainder = (((u32)len << 4) % 99);
			delta = 99 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 16 ){
				}
				else if ( (delta >= 16) && (delta < 32) ){
					*(service) |= BIT_7;
				}
				else if ( (delta >= 32) && (delta < 48) ){
					*(service) |= BIT_6;
				}
				else if ( (delta >= 48) && (delta < 64) ){
					*(service) |= (BIT_6 | BIT_7);
				}
				else if ( (delta >= 64) && (delta < 80) ){
					*(service) |= BIT_5;
				}
				else if ( (delta >= 80) && (delta < 96) ){
					*(service) |= (BIT_5 | BIT_7);
				}
				else if ( delta >= 96 ){
					*(service) |= (BIT_5 | BIT_6); 
				}
			}
			break;
			
		case 11:		// 55M bps
			*(len_in_us) = (u16)(((u32)len << 3)/55);
			remainder = (((u32)len << 3) % 55);
			delta = 55 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 8 ){
				}
				else if ( (delta >= 8) && (delta < 16) ){
					*(service) |= BIT_7;
				}
				else if ( (delta >= 16) && (delta < 24) ){
					*(service) |= BIT_6;
				}
				else if ( (delta >= 24) && (delta < 32) ){
					*(service) |= (BIT_6 | BIT_7);
				}
				else if ( (delta >= 32) && (delta < 40) ){
					*(service) |= BIT_5;
				}
				else if ( (delta >= 40) && (delta < 48) ){
					*(service) |= (BIT_5 | BIT_7);
				}
				else if ( delta >= 48 ){
					*(service) |= (BIT_5 | BIT_6);
				}
			}
			break;
			
		case 12:		// 60.5M bps
			*(len_in_us) = (u16)(((u32)len << 4)/121);
			remainder = (((u32)len << 4) % 121);
			delta = 121 - remainder;
			if ( remainder ){
				*(len_in_us) += 1;
				if ( delta < 16 ){
				}
				else if ( (delta >= 16) && (delta < 32) ){
					*(service) |= BIT_7;
				}
				else if ( (delta >= 32) && (delta < 48) ){
					*(service) |= BIT_6;
				}
				else if ( (delta >= 48) && (delta < 64) ){
					*(service) |= (BIT_6 | BIT_7);
				}
				else if ( (delta >= 64) && (delta < 80) ){
					*(service) |= BIT_5;
				}
				else if ( (delta >= 80) && (delta < 96) ){
					*(service) |= (BIT_5 | BIT_7);
				}
				else if ( (delta >= 96) && (delta < 112) ){
					*(service) |= (BIT_5 | BIT_6);
				}
				else if ( delta >= 112 ){
					*(service) |= (BIT_5 | BIT_6 | BIT_7);
				}
			}
			break;				
		default:
			printk(KERN_ERR "zd1205: Invalid RF module parameter\n");
			
	}
}


#define CTRL_SIZE	25
__inline unsigned long cfg_ctrl_setting(
	struct zd1205_private *macp,
	sw_tcb_t		*sw_tcb,
	wla_header_t	*wla_hdr,
	ctrl_setting_parm_t *setting_parms
)
{
	u8			tmp;
	u16			len = 0;
	u16			next_len = 0;

	u16			len_in_us;
	u16			next_len_in_us;
	u8			service;
	ctrl_set_t	*ctrl_set = sw_tcb->hw_ctrl;
    u32			curr_frag_len = setting_parms->curr_frag_len;
	u32			next_frag_len = setting_parms->next_frag_len;
	u8			preamble = setting_parms->preamble;
	//u8			encry_type = setting_parms->encry_type;
	//u8			vapid = setting_parms->vapid;
	u8			rate = setting_parms->rate;
	u8			bMgtFrame = 0;
	u8			bGroupAddr = 0;
	u8			EnCipher = ((wla_hdr->frame_ctrl[1] & WEP_BIT) ? 1 : 0);
	u16			FragNum = (wla_hdr->seq_ctrl[0] & 0x0F);
	card_Setting_t	*pCardSettting = &macp->card_setting;
	
	if ((wla_hdr->frame_ctrl[0] & 0x0c) == MANAGEMENT){
		bMgtFrame = 1;
		rate = dot11Obj.BasicRate;
	}	
	
	if (bMgtFrame){	
		if (wla_hdr->frame_ctrl[0] == PROBE_RSP){
			rate = 0x0;
			preamble = 0;
		}	
	}	
	
	/* Set the control-setting */
	if ((rate == RATE_1M) && (preamble == 1) ){ //1M && short preamble
		rate = RATE_2M;
	}	
	
    ctrl_set->ctrl_setting[0] = (rate | (preamble << 5));
	rate = ctrl_set->ctrl_setting[0] & 0x1f;

	//keep current Tx rate
	pCardSettting->CurrTxRate = rate;

	/* Length in byte */
	if (EnCipher) {
		if (!pCardSettting->SwCipher){
			switch(pCardSettting->EncryMode){
				case WEP64: /* WEP 64 */
				case WEP128:
					len = curr_frag_len + 36; 	/* Header(24) + CRC32(4) + IV(4) + ICV(4) */
					next_len = next_frag_len + 36;
					break;
			
				default:
                	printk(KERN_DEBUG "encry_type = %x\n", pCardSettting->EncryMode);
                	break;
			}
		}
		else { //use software encryption
			if (pCardSettting->DynKeyMode == DYN_KEY_TKIP){
				if ((wla_hdr->DA[0] & BIT_0) && (pCardSettting->WpaBcKeyLen != 32)) { //multicast
					len = curr_frag_len + 32; // Header(24) + CRC32(4) + IV(4), ICV was packed under payload
					next_len = next_frag_len + 32;
				}
				else {	
					len = curr_frag_len + 36; // Header(24) + CRC32(4) + IV(4) + ExtendIV(4), ICV was packed under payload
					next_len = next_frag_len + 36;
				}	
			}
			else {
				len = curr_frag_len + 32; // Header(24) + CRC32(4) + IV(4), ICV was packed under payload
				next_len = next_frag_len + 32;
			}		
		}		
	}
	else{
		len = curr_frag_len + 28; 	/* Header(24) + CRC32(4) */
		next_len = next_frag_len + 28;
	}

	/* Corret some exceptions */
	if (next_frag_len == 0){
		next_len = 0;
	}

	ctrl_set->ctrl_setting[1] = (u8)len; 			/* low byte */
	ctrl_set->ctrl_setting[2] = (u8)(len >> 8);   /* high byte */

	/* TCB physical address */
	ctrl_set->ctrl_setting[3] = (u8)(sw_tcb->tcb_phys);
	ctrl_set->ctrl_setting[4] = (u8)(sw_tcb->tcb_phys >> 8);
	ctrl_set->ctrl_setting[5] = (u8)(sw_tcb->tcb_phys >> 16);
	ctrl_set->ctrl_setting[6] = (u8)(sw_tcb->tcb_phys >> 24);
	ctrl_set->ctrl_setting[7] = 0x00;
	ctrl_set->ctrl_setting[8] = 0x00;
	ctrl_set->ctrl_setting[9] = 0x00;
	ctrl_set->ctrl_setting[10] = 0x00;

	/* Misc */
	tmp = 0;
	if (!FragNum)
		tmp |= BIT_0;
		
	if (wla_hdr->DA[0] & BIT_0){		/* Multicast */
		bGroupAddr = 1;
		tmp |= BIT_1;
	}	

	//if (bMgtFrame){
	//	tmp |= BIT_3;
	//}

	//if (wla_hdr->frame_ctrl[0] == PS_POLL){
	//	tmp |= BIT_2;
	//}

	if (len > pCardSettting->RTSThreshold){
		if ((!bMgtFrame) && (!bGroupAddr))
			tmp |= BIT_5;
	}
	
	if ((EnCipher) && (!pCardSettting->SwCipher)){
		tmp |= BIT_6;
	}	

	ctrl_set->ctrl_setting[11] = tmp;

	/* Address1 */
    ctrl_set->ctrl_setting[12] = wla_hdr->DA[0];
    ctrl_set->ctrl_setting[13] = wla_hdr->DA[1];
    ctrl_set->ctrl_setting[14] = wla_hdr->DA[2];
    ctrl_set->ctrl_setting[15] = wla_hdr->DA[3];
    ctrl_set->ctrl_setting[16] = wla_hdr->DA[4];
    ctrl_set->ctrl_setting[17] = wla_hdr->DA[5];
  

	/* next_len */
		ctrl_set->ctrl_setting[18] = (u8)next_len;
		ctrl_set->ctrl_setting[19] = (u8)(next_len >> 8);

	/* len_in_us */
	cal_us_service(rate, len, &len_in_us, &service);
	ctrl_set->ctrl_setting[20] = (u8)len_in_us;
	ctrl_set->ctrl_setting[21] = (u8)(len_in_us >> 8);

	/* service */
	ctrl_set->ctrl_setting[22] = service;
	cal_us_service(rate, next_len, &next_len_in_us, &service);

	ctrl_set->ctrl_setting[23] = (u8)next_len_in_us;
	ctrl_set->ctrl_setting[24] = (u8)(next_len_in_us >> 8);
	
	return(CTRL_SIZE);
}


__inline u32 cfg_mac_header (
	struct zd1205_private	*macp,
	sw_tcb_t	 	*sw_tcb,
	wla_header_t 	*wla_hdr,
	u8				hdrLen
)
{
    
	header_t	*pHdr;
	u8			hdr_len;
    
	pHdr = sw_tcb->hw_header;
	hdr_len = hdrLen;
	memcpy(&pHdr->mac_header[0], (u8 *)wla_hdr, hdrLen);
 
    if (!macp->card_setting.SwCipher){ //hardware encryption
		if (pHdr->mac_header[1] & WEP_BIT){ //WEP bit is set
			/* Set IV only for Encryption mode */
			switch(macp->card_setting.EncryMode){
           		default:
					break;
			
				case WEP64: /* WEP 64 */
				case WEP128:
			    	macp->wep_iv++;
		       		pHdr->mac_header[24] = (u8)(macp->wep_iv >> 16);
           			pHdr->mac_header[25] = (u8)(macp->wep_iv >> 8);
			    	pHdr->mac_header[26] = (u8)macp->wep_iv;
					pHdr->mac_header[27] = (macp->card_setting.EncryKeyId << 6);
				    hdr_len += IV_SIZE;
				    break;
			}
		}
	}
	
	return(hdr_len);
}

#endif





