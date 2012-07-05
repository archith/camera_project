#ifndef _LOCK_API_H_
#define _LOCK_API_H_

#include <sys/uio.h>

enum{
	LOCK_FILE,	// sender obtains data from file
	LOCK_MEMORY	// sender obtains data from memory address
};

/* attachment lock shared by smtpc thread and the calling thread */
/* key for the thread to access lock_info */
typedef struct lock_key{
	int lock_state;	// LOCK_CTRL_STATE: calling thread operating on attachments
			// LOCK_SEND_STATE: smtpc thread sending out attachments
	int lock_cnt;	// number of lock loop after init
	int sender_num;	// total number of senders
	int ctrl_error;	// controller error, should abort
	int send_error;	// sender error, the sender causes the error should decrease sender_num
}LOCK_KEY;

typedef struct lock_data{
	int finish;	// this is the last data to be sent
	int data_type;	// sender obtains data from file or memory
	char* fname;// file name
	struct iovec* iov_ary;
	int ary_num;
	//char* data_address;// memory address of data
	//size_t data_len;// size of data (in bytes)
}LOCK_DATA;

typedef struct lock_info{
	LOCK_KEY key;
	LOCK_DATA data;
}LOCK_INFO;

/* lock_init:
 * Description: called by controller to initial the lock value
 */
void lock_init(void);

/* lock_destroy:
 * Description: called by controller to release the lock resource
 */
void lock_destroy(void);

/* lock_set_controller_error:
 * Description: called by controller or senders to set the error flag
 */
void lock_set_controller_error(void);

/* lock_set_sender_error:
 * Description: called by sender to set the error flag and decrease the sender_num
 */
int lock_set_sender_error(void);

/* lock_setup_controller:
 * Description: called by controller to setup some infomation
 * Parameter:	sender_num - total number of senders
 */
void lock_setup_controller(int sender_num);

/* controller_capture_lock:
 * Description: called by controller to capture the lock
 */
int controller_capture_lock(void);

/* controller_release_lock:
 * Description: called by controller to release the lock to senders
 */
int controller_release_lock(void);

/* sender_capture_lock:
 * Description: called by sender to capture the lock
 * Parameter:	lock_cnt - number of locks the sender enters after lock_init
 */
int sender_capture_lock(int lock_cnt);

/* sender_release_lock:
 * Description: called by sender to release the lock to controller
 */
int sender_release_lock(void);

#endif	// _SMTPC_LOCK_H_
