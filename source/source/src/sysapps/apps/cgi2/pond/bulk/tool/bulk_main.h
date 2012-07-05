#ifndef  _BULK_MAIN_H_
#define  _BULK_MAIN_H_

#include <cgi-parse.h>
#include <bulk_ops.h>
int bulk_ops_read_ds(char* name, void** ds, struct bulk_ops** bulk_ops_list);
int bulk_ops_read_ds_swap(char* name, void** ds, struct bulk_ops** bulk_ops_list);
int bulk_flag_on_ds(char* name, struct bulk_ops** bulk_ops_list, int flag);
int bulk_flag_off_ds(char* name, struct bulk_ops** bulk_ops_list, int flag);
int bulk_action_ds(struct bulk_ops** bulk_ops_list);
#endif
