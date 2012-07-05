#ifndef _LOCK_VAR_H_
#define _LOCK_VAR_H_


//#define _LOCK_DEBUG_
#ifdef _LOCK_DEBUG_
	#define DB(fmt, arg...);	\
	{\
		if(1) fprintf(stderr, fmt, ##arg);\
	}
#else
	#define DB(fmt, arg...);	{}
#endif


enum{
	LOCK_CTRL_STATE,	// when controller building file
	LOCK_SEND_STATE		// when sender sending file
};
#define LOCK_INIT	LOCK_CTRL_STATE



#endif // _LOCK_API_H_
