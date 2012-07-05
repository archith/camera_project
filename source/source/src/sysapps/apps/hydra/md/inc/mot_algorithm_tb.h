/***************************************************************
 * (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
 *
 *      Use of this software is restricted to the terms and
 *      conditions of SerComm's software license agreement.
 *
 *                        www.sercomm.com
 ****************************************************************/
#ifndef _MOT_ALGORITHM__TB_H_
#define _MOT_ALGORITHM_TB_H_

#include "maf_mot.h"
#if 0
static short sensTb_dec[SENS_NUM]=
{
        -100/* SENS 0  (-125) */,
        -50/* SENS 1  (-100) */,
        -30/* SENS 2  ( -75) */,
        -8/* SENS 3  ( -50) */,
        2/* SENS 4  ( -25) */,
        4/* SENS 5  (   0) */,
        8/* SENS 6  ( +25) */,
        0/* SENS 7  ( +50) */,
        0/* SENS 8  ( +75) */,
        0/* SENS 9  (+100) */,
        0/* SENS 10 (+125) */
};

static short sensTb[SENS_NUM]=
{
        1/* SENS 0  (255= 1*255 / 127=1*(127=227-100) */,
        1/* SENS 1  (255= 1*255)/ 127=1*(127=177-50) */,
        1/* SENS 2  (255= 1*255)/ 127=1*(127=157-30) */,
        2/* SENS 3  (255= 2*127)/ 127=2*(63=71- 8) */,
	2/* SENS 4  (255= 2*127)/ 127=2*(63=63+ 0) */,
        2/* SENS 5  (255= 2*127)/ 127=2*(63=59+ 4) */,
        2/* SENS 6  (255= 2*127)/ 127=2*(63=53+10) */,
        3/* SENS 7  (255= 4* 63)/ 127=4*31 */,
        5/* SENS 8  (255= 5* 51)/ 127=5*25 */,
        6/* SENS 9  (255= 6* 46)/ 127=6*21 */,
       10/* SENS 10 (255=10* 25)/ 150=15*10 */
};
#endif

#endif
