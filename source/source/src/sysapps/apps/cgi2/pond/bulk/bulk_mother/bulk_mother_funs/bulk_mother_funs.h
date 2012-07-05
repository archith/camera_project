/***************************************************************
* (C) COPYRIGHT SerComm Corporation All Rights Reserved, 2004.
*
*      Use of this software is restricted to the terms and
*      conditions of SerComm's software license agreement.
*
*                        www.sercomm.com
****************************************************************/
#ifndef  _BULK_MOTHER_FUNS_H_
#define  _BULK_MOTHER_FUNS_H_

int upnp_restart(struct bulk_ops** bulk_ops_list);
int bonj_restart(struct bulk_ops** bulk_ops_list);
int bonj_dont_run(struct bulk_ops** bulk_ops_list);
int network_restart(struct bulk_ops** bulk_ops_list);
int wlan_restart(struct bulk_ops** bulk_ops_list);
int check_port_conflict(struct bulk_ops** bulk_ops_list);
int todo_action(struct bulk_ops** bulk_ops_list);
#endif

