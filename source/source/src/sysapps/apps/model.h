/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2006.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef _MODEL_H_
#define _MODEL_H_

/* ---- Put all of the available OEM names here ---- */
#define OEM_SerComm      0x0001
#define OEM_Linksys      0x0002	/* SMB? Retail? */
#define OEM_Planet       0x0003

#define OEM              OEM_Linksys	/* Assign the OEM customer */
#undef _MODEL_

#if (OEM == OEM_SerComm)
#define NC802APT         0x0001
#define NC802A           0x0002
#define NC802AN          0x0003
#define NC402A           0x0004
#define NC402AN          0x0005
#define M_MAX_OEM1       0x0020	/* M_MAX_OEM1+1 = [Linksys] 1st Model */

#define _MODEL_    NC802APT	/* assign the current model for this OEM */
#endif /* SerComm */

#if (OEM == OEM_Linksys)
#define WVC200           0x0021
#define WVC200_EU        0x0022
#define WVC54GPT         0x0023
#define WVC54GPT_EU      0x0033
#define M_MAX_OEM2       0x0040	/* M_MAX_OEM1+1 = [Planet] 1st Model */

#define _MODEL_    WVC200	/* assign the current model for this OEM */
#endif /* Linksys */


#if (OEM == OEM_Planet)
#define ICA-550W         0x0041
#define M_MAX_OEM3       0x0050	/* M_MAX_OEM1+1 = [??] 1st Model */

#define _MODEL_    ICA-550W	/* assign the current model for this OEM */
#endif /* Planet */

#ifndef _MODEL_
#error  ERROR! Not assign any model type for this build!!
#endif

/* -------- Error Check -------- */
#if (_MODEL_ <= M_MAX_OEM1)
#if (OEM != OEM_SerComm)
#error  ERROR! Incorrect Model Number for this OEM!!
#endif /* OEM_SerComm */
#elif ((_MODEL_ > M_MAX_OEM1) && (_MODEL_ <= M_MAX_OEM2))
#if (OEM != OEM_Linksys)
#error  ERROR! Incorrect Model Number for this OEM!!
#endif /* OEM_Linksys */
#elif ((_MODEL_ > M_MAX_OEM2) && (_MODEL_ <= M_MAX_OEM3))
#if (OEM != OEM_Planet)
#error  ERROR! Incorrect Model Number for this OEM!!
#endif /* OEM_Planet */
#else
#error  ERROR! Unknown Model Number for this OEM!!
#endif

#endif /* _MODEL_H_ */
