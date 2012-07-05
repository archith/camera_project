/*
 * os-glue/fw_os.h
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 

#ifndef _FW_OS_H_
#define _FW_OS_H_

#define MLME_LINUX

/* Linux driver version */
#ifdef MLME_LINUX
#include "fw_linux.h"
#endif

/* ADS version */
#ifdef MLME_ADS
#include "sim/fw_ads.h"
#endif

/* template version */
#ifdef MLME_TEMPLATE
#include "00_template/os_template.h"
#endif

#endif
