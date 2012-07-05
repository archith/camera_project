#ifndef _DNSWITCH_API_H_
#define _DNSWITCH_API_H_

#include <dnswitch_bulk_ops.h>

#define DN_FILTER_DAY		0
#define DN_FILTER_NIGHT		1
int DNSwitchAction(struct DNSWITCH_DS* info);
int DNSwitchManual(int dn_filter);
#endif
