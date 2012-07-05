#ifndef __ZDGLOBAL_C__
#define __ZDGLOBAL_C__

#include "zd80211.h"


U8			mPreambleType = LONG_PREAMBLE;
MacAddr_t	dot11MacAddress = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};
Element		dot11DesiredSsid;
U8			mAuthAlogrithms[2] = {OPEN_SYSTEM, SHARE_KEY};
U16			mRfChannel = 0;				
U16			mBeaconPeriod = 100;		
U16			mDtimPeriod = 1;
U16			mFragThreshold = 2432;
U16 		mRtsThreshold = 2432;


//WPA
Element		mWPAIe;


//WEP
U8			mKeyId = 0;
U8			mKeyFormat = WEP64_USED;
BOOLEAN 	mPrivacyInvoked = FALSE;

Element		mSsid;				
Element		mBrates;
Element 	mPhpm;
MacAddr_t	mBssId;				
U16 		mCap = CAP_ESS;				
U16 		mDtimCount;			

U8	 		mPsStaCnt = 0;	//Station count for associated and in power save mode
U8			mHiddenSSID = 0;
U8			mLimitedUser = 0;
U8			mCurrConnUser = 0;
U8			mBlockBSS = 0;
U8			mRadioOn = 1;
U8			mSwCipher = 0;
U8			mKeyVector[4][16];
U8			mBcKeyVector[16];
U8 			mWepIv[4];
U8 			mBcIv[4];
U8			mWepKeyLen;
U8			mBcKeyLen;
U8			mBcKeyId;
U8			mDynKeyMode = 0;
BOOLEAN		mZyDasModeClient = FALSE;
Seedvar		mBcSeed;
MICvar		mBcMicKey;
U8			mWpaBcKeyLen = 32;
U8			mWpaBcKeyId = 1;
U8			mGkInstalled = 0;
U16			mIv16 = 0;
U32			mIv32 = 0;
#endif
