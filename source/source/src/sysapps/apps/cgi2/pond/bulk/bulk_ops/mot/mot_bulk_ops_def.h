#ifndef _MOT_BULK_OPS_DEF_H_
#define _MOT_BULK_OPS_DEF_H_

#include <mot_bulk_ops.h>
static struct motion_str mot_ds;
static struct motion_str mot_ds_rg;
struct bulk_ops mot_bulk_ops=
{
        name:		MOT_BULK_NAME,
        read:       ReadMotConf,
        check:      Mot_CheckConf,
        write:      Mot_WriteConf,
        run:        Mot_RunConf,
        ds:         &mot_ds,
        ds_org:     &mot_ds_rg,
        ds_size:	sizeof(struct motion_str),
        flag:       BULK_DEFAULT
};

#endif
