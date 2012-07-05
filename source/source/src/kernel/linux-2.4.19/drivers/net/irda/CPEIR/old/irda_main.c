/***************************************************************************
* Copyright  Faraday Technology Corp 2002-2003.  All rights reserved.      *
*--------------------------------------------------------------------------*
* Name: irda_FIR_Test2.c                                                   *
* Description:                                                             *
* Author: Bruce                                                            *
* Date: 2002/10/15                                                         *
* Version:1.0                                                              *
*--------------------------------------------------------------------------*
****************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fLib.h"
#include "cpe_serial.h"   
#include "irda.h"
#include "APB_DMA.h"
#include "irda_Test.h"
#include "fa510.h"
//=========================================================================================
// Function Name: main
// Description: 
//=========================================================================================
int main(int argc, int *argv[])
{
UINT8 item;






do{
   
   printf("====================== IrDA Function Test ========================\n");
   printf("== 1:SIR-PIO Test(Free Run Test)     CPE X1                     ==\n");
   printf("== 2:SIR-PIO Test(Free Run Test)     CPE X2                     ==\n");
   printf("== 3:SIR-DMA Test(Free Run Test)     CPE X1                     ==\n");   
   printf("== 4:FIR-PIO Test(Free Run Test)     CPE X2                     ==\n");   
   printf("== 5:FIR-DMA Test(Free Run Test)     CPE X2                     ==\n");   
   printf("== 6:Other FIR Test Function         CPE X2                     ==\n");      
   printf("==--------------------------------------------------------------==\n");        
   printf("== PS: CPE X1 => (Role-A:Use CPE1-IRDA1, Role-B:Use CPE1-IRDA2) ==\n");        
   printf("==     CPE X2 => (Role-A:Use CPE1-IRDA1, Role-B:Use CPE2-IRDA2) ==\n");
   printf("==================================================================\n");
   printf("Please Input the Test Item:");
   scanf("%d",&item);
   switch(item)
      {

        case 1: //
                fLib_IrDA_AutoMode_SIR_Low(0x98200000);
                fLib_IrDA_AutoMode_SIR_Low(0x98300000);

                SIR_Test_RreeRun_PIO();
        break;

        case 2: //
                fLib_IrDA_AutoMode_SIR_Low(0x98200000);
                fLib_IrDA_AutoMode_SIR_Low(0x98300000);

                SIR_Test_RreeRun_PIO_2();
        break;

        case 3: //
                fLib_IrDA_AutoMode_SIR_Low(0x98200000);
                fLib_IrDA_AutoMode_SIR_Low(0x98300000);

                SIR_Test_RreeRun_DMA();
        break;
        case 4: //
                fLib_IrDA_AutoMode_FIR_High(0x98200000); 
                fLib_IrDA_AutoMode_FIR_High(0x98300000); 
                FIR_Test12_Main_Auto_Free_Run_PIO();
        break;
        case 5: //
                fLib_IrDA_AutoMode_FIR_High(0x98200000); 
                fLib_IrDA_AutoMode_FIR_High(0x98300000); 
                FIR_Test12_Main_Auto_Free_Run_DMA();
        break;
        case 6: //
                fLib_IrDA_AutoMode_FIR_High(0x98200000); 
                fLib_IrDA_AutoMode_FIR_High(0x98300000); 
                main_FIR();
        break;
     
     
      };     
   
   }while(1);

    return TRUE;

}

