#define CMD_SHOWHELPMSG				0
#define CMD_SNAPSHOT				1

typedef struct {
	int cmd;		// command type
	int res;		// resolution
	int qual;		// quality
	int ctr;		// snapshot counter
} SnapshotConfigure;

/****************************************************************** 
	Function:		showHelpMsg()

	Description:	Show the Help message

	Input:			None
	Output:			None
	Return:			None
 *******************************************************************/ 
void showHelpMsg();
