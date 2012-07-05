//#define _IRIS_DBMSG_

#define YES				1
#define NO				0

// value definition
#define DVALUE_OF_VITEM	-1

#define MIN_KEEP		1
#define MID_KEEP		3
#define MAX_KEEP		8	

#define MIN_DAC_NUM		0
#define MAX_DAC_NUM		90


#define PULL_UP_VOLTAGE_IF_FINDING_VALID_MODE				3
#define PULL_UP_VOLTAGE_IF_NEXT_ADJUSTMENT_COULD_BE_TOO_FAR	3
#define PULL_UP_VOLTAGE_IF_FINDING_VALID_MODE				3


#define BASIC_REG_UNIT	1

#define INVALID_MODE_UNIT	5
#define VALID_MODE_UNIT		1
#define BASIC_UNIT			1


#define MINUS_ONE_UNIT				3
#define ENTER_VALID_MODE			2
#define EXPOSURE_IS_OK				0
#define EXPOSURE_IS_TOO_HIGH		1
#define EXPOSURE_IS_TOO_LOW			-1
#define EXPOSURE_IS_VERY_LOW		-2


#define EXPOSURE_RANGE				30


#define INVALID	0
#define VALID	1



struct semaphore sem;
int CurrentIRISIndicator = 0;

int CurrentIRISIndex = 0;

int valid_mode = 0;		// enter the valid mode if the valid voltage is found
int emergency_stop = 0;	// emergency stop because the iris could close too much
int temporal_stop = 0;	// temporal stop because we need to check something

int extra_voltage_to_stop = 0;	//


int dac_first_register_value = 20;		// the first register value we set
int dac_second_register_value = 12;		// the second register value we set
int last_valid_value = 0;			// record the latest valid that we found

int keep = MIN_KEEP;	// how many times should we keep the current voltage
int tried = 0;			// how many times did we use the current voltage


int failed_and_tolerance_range = 0;

int autoiris_state = 0;


/***** state control *****/

static void state_init()
{
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(50);

	return 0;
}



static int CheckCurrentAvgY(int cur, int thr)
{
	if(cur > thr - EXPOSURE_RANGE && cur < thr + EXPOSURE_RANGE + failed_and_tolerance_range * 2)
	{
		return EXPOSURE_IS_OK;
	}
	else if(cur >= thr + EXPOSURE_RANGE)
	{
		return EXPOSURE_IS_TOO_HIGH;
	}
	else if(cur < thr - (EXPOSURE_RANGE * 3) / 2)
	{
		return EXPOSURE_IS_VERY_LOW;
	}
	else
	{
		return EXPOSURE_IS_TOO_LOW;
	}
}



static void cal_dac_register(int c, int mode)
{
	if(dac_first_register_value < 20)
	{
		if(last_valid_value > 20)
		{
			dac_first_register_value = last_valid_value - 5;
			dac_second_register_value = dac_first_register_value - 6;

		}
		else
		{
			dac_first_register_value = 20;
			dac_second_register_value = 12;
		}
	}

	switch(c)
	{
		case MINUS_ONE_UNIT:
			dac_first_register_value -= BASIC_UNIT;
			dac_second_register_value -= BASIC_UNIT; 
			break;

		
		case ENTER_VALID_MODE:
			dac_first_register_value -= PULL_UP_VOLTAGE_IF_FINDING_VALID_MODE;
			dac_second_register_value -= PULL_UP_VOLTAGE_IF_FINDING_VALID_MODE; 

			#ifdef _IRIS_DBMSG_
			printk("VMODE: %d REG(%d,%d)\n", valid_mode, dac_first_register_value, dac_second_register_value);
			#endif

			break;


		case EXPOSURE_IS_TOO_HIGH:
			if(tried > 0)
			{
				tried--;
				break;
			}

			if(dac_first_register_value < dac_second_register_value+5)
			{
				if(mode == VALID)
				{
					dac_first_register_value += VALID_MODE_UNIT;
					tried = MID_KEEP;
				}
				else 
				{
					dac_first_register_value += INVALID_MODE_UNIT;
					last_valid_value = dac_first_register_value;
					tried = MAX_KEEP;
				}

				dac_second_register_value = dac_first_register_value-6;
			}
			else
			{
				if(mode == VALID)
				{
					dac_second_register_value += VALID_MODE_UNIT;
				}
				else
				{
					dac_second_register_value += INVALID_MODE_UNIT;
				}
			}

			break;
		
		case EXPOSURE_IS_TOO_LOW:
			if(tried > 0)
			{
				tried--;
			}
			else
			{
				dac_first_register_value -= 2;
				dac_second_register_value = dac_first_register_value - 6;
				tried = MID_KEEP;
			}

			break;

		case EXPOSURE_IS_VERY_LOW:
			if(tried > 0)
			{
				tried--;
				break;
			}

			dac_first_register_value -= 4;
			dac_second_register_value = dac_first_register_value - 6;
			tried = MID_KEEP;
			break;

		case EXPOSURE_IS_OK:
		default:
			break;
	}

	if(dac_first_register_value < MIN_DAC_NUM)
	{
		dac_first_register_value = MIN_DAC_NUM;
	}

	if(dac_first_register_value >= MAX_DAC_NUM)
	{
		dac_first_register_value = MAX_DAC_NUM;
	}

	if(dac_second_register_value < dac_first_register_value - 6)
	{
		dac_second_register_value = dac_first_register_value - 6;
	}
}

