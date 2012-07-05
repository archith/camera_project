#ifndef  _BULK_OPS_H_
#define  _BULK_OPS_H_

enum
{
	BULK_DEFAULT = 0x1,//BULK_READ_STATE
	BULK_CHECK_STATE = 0x2,
	BULK_WRITE_STATE = 0x4,
	BULK_RUN_STATE = 0x8,
};
struct bulk_ops
{
	char *name;
	int (*read)(void *ds);
	int (*check)(void* ds, void* ds_org);
	int (*write)(void *ds, void *ds_org);
	int (*run)(void *ds, void *ds_org);
	int (*web_msg)(int errcode, char* message, int* type);
	void *ds;
	void *ds_org;
	int ds_size;
 	int flag;
};
#define BULK_OPS_MESSAGE_FLAG	0x10000
/*	
char *name;
	An unique name for the bulk ops

int (*read)(void *ds);
	read settings into "ds"
	0: sucess
	-1: error

int (*check)(void* ds, void* ds_org);
	check whether "ds" is legal or not
	"ds_org" can help "ds" to find out modified value
	1: sucess, there are some modified items 
	0: sucess, no modified items
	-1: error

int (*write)(void *ds, void *ds_org);
	write "ds" to your settings
	"ds_org" can help you to write modified items only.
	0: sucess
	-1: error

int (*run)(void *ds, void *ds_org);
	run settings according to "ds"
	0: sucess
	-1: error
	
int (*web_msg)(int errcode, char* message, int* type);
	when "read", "check", "write" and "run" return -1 or "BULK_OPS_MESSAGE_FLAG" oring any interger,
	the function will be called to fill out some settings.
	0: sucess
	-1: error

void *ds;
void *ds_org;
int ds_size;

int flag;
	The default value should be BULK_DEFAULT_STATE
*/

#endif

