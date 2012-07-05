#ifndef _ALL_BULK_MOTHER_OPS_H_
#define _ALL_BULK_MOTHER_OPS_H_

#include <bulk_ops.h>
#include <rtsp_bulk_mother_def.h>
#include <web_bulk_mother_def.h>
#include <system_bulk_mother_def.h>
#include <qos_bulk_mother_def.h>
#include <network_bulk_mother_def.h>
#include <misc_bulk_mother_def.h>

#define ALL_BULK_MOTHER_OPS_LIST {&rtsp_bulk_mother_ops, &web_bulk_mother_ops, \
	&sys_bulk_mother_ops, &qos_bulk_mother_ops, &network_bulk_mother_ops, \
	&misc_bulk_mother_ops, NULL}
#endif
