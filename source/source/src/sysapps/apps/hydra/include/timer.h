#ifndef __TIMER_H__
#define __TIMER_H__

/* 
  active timer lives in grabber thread so that the minimal time grain
  (in normal condiction) is 33 ms, where passive timer work only if a
  use call check_timer_expired.
  Every time a new frame is generated, timer array will be process once,
  since it parasite in the grabber thread, so that I don't suggest the callback
  function takes a lot of time to complete its work
 */

struct timer_s
{
	unsigned int instance_id;	/* identify an instance */
	time_t	create_time;		/* when this timer was created */
	time_t	last_check_time;	/* update everytime when 
					   check_timer_expired called */
	time_t	expire_duration;	/* user define expiration time */
	time_t	expire_time;		/* expire absolute time, update
					   when check_timer_expired call
					   and timer is expired */
	int (*func)(void); /* timer callback function */
//	int (*func)(int argc, char **argv); /* timer callback function */
	unsigned int	flags;		/* active or passive */
};

#define MAX_TIMER_INSTANCE	16

struct timer_s	timer[MAX_TIMER_INSTANCE];


#define TIMER_ACTIVE	0x01	/* an active timer will be checked automatically 
				   in the grabber thread */
#define TIMER_PASSIVE	0x02	/* a passive timer has to be checked only when the
				   user calls the check_timer_expired */

/* create a timer instance:
	return 0, ok; return -1, failed.
	when return true, an unique id will be filled.
	expire, expire duration, counts in msec.
	flags, TIMER_ACTIVE or TIMER_PASSIVE
	func
*/

int create_timer(unsigned int *id, time_t expire, unsigned int flags, int (*func)(void));

/* delete a timer instance 
	return 0, ok; return -1, failed.
	id, the timer instance to be deleted.
*/

int delete_timer(unsigned int id);

/* check a timer instance 
	return 1, expire; return 0, false.
	id, the timer instance to be checked.
*/
int check_timer_expired(unsigned int id);
#endif
