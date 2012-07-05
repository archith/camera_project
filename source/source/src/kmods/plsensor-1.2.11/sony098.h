#ifndef __SONY098_H__
#define __SONY098_H__

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <video/plsensor.h>


typedef struct _tagSONY098Data 
{
	struct semaphore	UpdateLock;
	devfs_handle_t		DevFS;
	PSENSOR				pSensor;
} SONY098DATA;

typedef SONY098DATA *PSONY098DATA;

#endif
