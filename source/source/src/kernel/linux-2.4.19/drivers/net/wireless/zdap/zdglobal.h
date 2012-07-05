#ifndef __ZDGLOBAL_H__
#define __ZDGLOBAL_H__

extern U16			mRfChannel;
extern U16			mDtimPeriod;
extern U16			mBeaconPeriod;
extern Element		dot11DesiredSsid;
extern U8		 	mAuthAlogrithms[2];
extern U8			mPreambleType;


extern BOOLEAN 		mPrivacyInvoked;
extern U8 			mKeyId;
extern U8 			mBcKeyId;
extern U8			mKeyFormat;
extern MacAddr_t	dot11MacAddress;
extern U16 			mRtsThreshold;
extern U16			mFragThreshold;


//WPA
//extern Element 		mWPA;
extern Element		mWPAIe;


extern U16 			mCap;		
extern U16 			mDtimCount;	
extern Element		mSsid;		
extern Element		mBrates;	
extern Element 		mPhpm;
extern MacAddr_t 	mBssId;	

//feature	
extern U8 			mPsStaCnt;
extern U8			mHiddenSSID;
extern U8			mLimitedUser;
extern U8			mCurrConnUser;
extern U8			mBlockBSS;
extern U8			mRadioOn;
extern U8			mSwCipher;
extern U8			mKeyVector[4][16];
extern U8			mBcKeyVector[16];
extern U8 			mWepIv[4];
extern U8 			mBcIv[4];
extern U8			mWepKeyLen;
extern U8			mBcKeyLen;
extern U8			mDynKeyMode;
extern BOOLEAN		mZyDasModeClient;
extern Seedvar		mBcSeed;
extern MICvar		mBcMicKey;
extern U8			mWpaBcKeyLen;
extern U8			mWpaBcKeyId;
extern U8			mGkInstalled;
extern U16			mIv16;
extern U32			mIv32;
#endif
