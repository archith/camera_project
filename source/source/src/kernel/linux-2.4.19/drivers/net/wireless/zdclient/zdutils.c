#ifndef __ZDUTIL_C__
#define __ZDUTIL_C__
#include "zd80211.h"

BOOLEAN	getElem(Frame_t	*frame, ElementID  eleID, Element  *elem)
{
	U8 k = 0; 	//offset bytes to first element
	U8 n = 0; 	//num. of element
	U8 pos;		//current position
	
	switch (frmType(frame)){
		case ST_BEACON:
		case ST_PROBE_RSP:
			k = 12;
			n = 7;
			break;
		
		case ST_PROBE_REQ:
			k = 0;
			n = 2;
			break;
		
		case ST_AUTH:
			k = 6;
			n = 1;
			break;
			
		case ST_ASOC_REQ:
			k = 4;
			n = 2;
			if (mDynKeyMode == DYN_KEY_TKIP)	
				n++;
			break;
			
		case ST_REASOC_REQ:
			k= 10;
			n= 2;
			if (mDynKeyMode == DYN_KEY_TKIP)	
				n++;
			break;
			
		case ST_ASOC_RSP:
		case ST_REASOC_RSP:
			k = 6;
			n = 1;
			break;
	}

	while(n--){
		pos = frame->body[k]; 
		if (pos == eleID){	//match
			U8 len = frame->body[k+1] + 2;
			if (len > sizeof(Element))
				return FALSE;

			memcpy((U8 *)elem, &frame->body[k], len);
			return TRUE;
		}
		else{
			k += frame->body[k+1] + 2;
		}
	}
	
	return FALSE;
}


//make information element
U16 addElem(U8 *body, int i, Element *elem)
{
	U8 len; 
	
	if (!elem)
		return 0;
		
	len = eLen(elem) + 2;
	memcpy((char*)&(body[i]), (char*)elem, len);
	return len;
}	


//make management frame
void mkMgtFrame(
	Fdesc_t* fdesc,
	TypeSubtype subType,
	MacAddr_t *addr1,
	MacAddr_t *addr2,
	MacAddr_t *addr3,
	MacAddr_t *addr4,
	...)
{
	va_list ap;
	int k;
	U8 *body;
	
	Frame_t *pf = &fdesc->mpdu[0];
	setFrameControl(pf, subType);
	pf->body = &fdesc->buffer[REVERED_LEN_8023];
	body = pf->body;
	if (addr1) 
		setAddr1(pf, addr1);
		
	if (addr2) 
		setAddr2(pf, addr2);
		
	if (addr3) 
		setAddr3(pf, addr3);
		
	if (addr4) 
		setAddr4(pf, addr4);
		
	va_start(ap, addr4); //make ap point to 1st unamed arg

	k = 0;
	switch(subType){
		case ST_AUTH:
			{
				U16 alg = va_arg(ap, U32);
				U16 seq = va_arg(ap, U32);
				U16 status = va_arg(ap, U32);
				
				mk16(body, 0, alg); //AuthAlg
				mk16(body, 2, seq); //AuthSeq
				mk16(body, 4, (seq == 2||seq == 4) ? status : (U16)0);
				k = 6;
				if ((alg == SHARE_KEY) && (seq == 2 || seq == 3)){
					mk8(body, k, EID_CTEXT);
					k++;
					mk8(body, k, CHAL_TEXT_LEN);
					k++;
					memcpy(body+k, va_arg(ap, U8*), CHAL_TEXT_LEN);
					k += CHAL_TEXT_LEN;
				}
			}
			break;
			
		case ST_ASOC_RSP:
		case ST_REASOC_RSP:
			mk16(body, 0, va_arg(ap, U32)); //Cap
			mk16(body, 2, va_arg(ap, U32)); //Status
			mk16(body, 4, va_arg(ap, U32)); //AID
			k += 6;
			k += addElem(body, k, va_arg(ap, Element*)); //Support Rates
			break;
			
		case ST_PROBE_RSP:
			mk16(body, 8, va_arg(ap, U32));  //BcnPeriod
			mk16(body, 10, va_arg(ap, U32)); //Cap
			k += 12;
			k += addElem(body, k, va_arg(ap, Element*)); //SSID
			k += addElem(body, k, va_arg(ap, Element*)); //Support Rates
			k += addElem(body, k, va_arg(ap, Element*)); //Phy Parms
			if (mDynKeyMode == DYN_KEY_TKIP)
				k += addElem(body, k, va_arg(ap, Element*)); //WPA IE
			break;
			
		case ST_DISASOC:
			mk16(body, 0, va_arg(ap, U32)); //Reson code
			k = 2;
			break;
			
		case ST_DEAUTH:
			mk16(body, 0, va_arg(ap, U32)); //Reson code
			k = 2;
			break;
			
		default:
			break;
	}
	va_end(ap);
 

	if (addr4)
		hdrLen(pf) = WDS_ADD_LNG + MAC_HDR_LNG;
	else
		hdrLen(pf) = MAC_HDR_LNG;		
		
	bodyLen(pf) = k;
}

#endif
