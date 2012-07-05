/*
 * os-glue/fw_ads.c
 *
 * Copyright (C) 2004  Ivan Wang <ivan@faraday.com.tw>
 */ 

#include "fw_linux.h"
#include <linux/random.h>

/* provide random value */
unsigned char fw_rand_bytes()
{
    unsigned char buf;
    get_random_bytes(&buf,1);
    return buf;
}

/* ms unit */
void fw_setup_timer(void *timer,unsigned int period,void *data,void *func)
{
    struct timer_list *t=(struct timer_list *)timer;
//fw_printf(MLME_DBG_LEVEL1,"fw_setup_timer\n");
	t->data = (unsigned int)data;
	t->expires = jiffies + period;
	t->function = func;
}

void fw_init_timer(void *timer)
{
    struct timer_list *t=(struct timer_list *)timer;
//fw_printf(MLME_DBG_LEVEL1,"fw_init_timer\n");    
    if(t)
        init_timer(t);
}


void fw_add_timer(void *timer)
{
    struct timer_list *t=(struct timer_list *)timer;
//fw_printf(MLME_DBG_LEVEL1,"fw_add_timer\n");    
    if(t)
    {
        fw_del_timer(t);
        add_timer(t);
    }
}

void fw_del_timer(void *timer)
{
    struct timer_list *t=(struct timer_list *)timer;
    del_timer(t);
}

