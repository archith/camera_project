#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/ioctl.h>

#define REGDEBUG_QUANTUM  4000 /* use a quantum size like scull */
#define REGDEBUG_IOC_MAGIC  'K'

#define REGDEBUG_REG_READ_VIRT_B            _IOW(REGDEBUG_IOC_MAGIC,   1, regdebug_quantum)
#define REGDEBUG_REG_READ_PHY_B             _IOW(REGDEBUG_IOC_MAGIC,   2, regdebug_quantum)
#define REGDEBUG_REG_WRITE_VIRT_B           _IOW(REGDEBUG_IOC_MAGIC,   3, regdebug_quantum)
#define REGDEBUG_REG_WRITE_PHY_B            _IOW(REGDEBUG_IOC_MAGIC,   4, regdebug_quantum)
#define REGDEBUG_REG_READ_VIRT_W            _IOW(REGDEBUG_IOC_MAGIC,   5, regdebug_quantum)
#define REGDEBUG_REG_READ_PHY_W             _IOW(REGDEBUG_IOC_MAGIC,   6, regdebug_quantum)
#define REGDEBUG_REG_WRITE_VIRT_W           _IOW(REGDEBUG_IOC_MAGIC,   7, regdebug_quantum)
#define REGDEBUG_REG_WRITE_PHY_W            _IOW(REGDEBUG_IOC_MAGIC,   8, regdebug_quantum)

#define REGDEBUG_USER_READ_VIRT_B           _IOW(REGDEBUG_IOC_MAGIC,   9, regdebug_quantum)
#define REGDEBUG_USER_READ_PHY_B            _IOW(REGDEBUG_IOC_MAGIC,  10, regdebug_quantum)
#define REGDEBUG_USER_WRITE_VIRT_B          _IOW(REGDEBUG_IOC_MAGIC,  11, regdebug_quantum)
#define REGDEBUG_USER_WRITE_PHY_B           _IOW(REGDEBUG_IOC_MAGIC,  12, regdebug_quantum)
#define REGDEBUG_USER_READ_VIRT_W           _IOW(REGDEBUG_IOC_MAGIC,  13, regdebug_quantum)
#define REGDEBUG_USER_READ_PHY_W            _IOW(REGDEBUG_IOC_MAGIC,  14, regdebug_quantum)
#define REGDEBUG_USER_WRITE_VIRT_W          _IOW(REGDEBUG_IOC_MAGIC,  15, regdebug_quantum)
#define REGDEBUG_USER_WRITE_PHY_W           _IOW(REGDEBUG_IOC_MAGIC,  16, regdebug_quantum)

#define REGDEBUG_IODEFAULT                 _IOW(REGDEBUG_IOC_MAGIC,   17, regdebug_quantum)

#define MODEMDEVICE "/dev/Rdebug/reg"

int regdebug_quantum = REGDEBUG_QUANTUM;

int main(int argc, char **argv)
{	
  int fd,ch;
  int cmd = REGDEBUG_IODEFAULT;
  int   reg_v_p=0; //  reg_virt=1,   reg_phy=2
  int  user_V_P=0; // user_virt=1,  user_phy=2
  int byte_word=0; //      byte=1,      word=2
  int   arg_r_w=0; //      read=1,     write=2
  
  typedef struct reg_test_s{
    unsigned int target_addr;
    unsigned int register_value;
  }reg_test_t; 
  
  reg_test_t args;
  memset(&args, 0, sizeof(reg_test_t));
  optarg = NULL;
  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
  if(fd<0){
            printf("open device /dev/Rdebug/reg fail \n");
            return 0;
          }

  while ((ch = getopt(argc, argv, "rwvpVPa:n:hBW")) != -1)
  {
    switch(ch)
    {
      case 'r':
                if( reg_v_p==1&&byte_word==1)  cmd = REGDEBUG_REG_READ_VIRT_B;
           else if( reg_v_p==1&&byte_word==2)  cmd = REGDEBUG_REG_READ_VIRT_W;
           else if( reg_v_p==2&&byte_word==1)  cmd = REGDEBUG_REG_READ_PHY_B;
           else if( reg_v_p==2&&byte_word==2)  cmd = REGDEBUG_REG_READ_PHY_W;
           else if(user_V_P==1&&byte_word==1)  cmd = REGDEBUG_USER_READ_VIRT_B;
           else if(user_V_P==1&&byte_word==2)  cmd = REGDEBUG_USER_READ_VIRT_W;
           else if(user_V_P==2&&byte_word==1)  cmd = REGDEBUG_USER_READ_PHY_B;
           else if(user_V_P==2&&byte_word==2)  cmd = REGDEBUG_USER_READ_PHY_W;
                arg_r_w=1;                       
                break;

      case 'w':
                if( reg_v_p==1&&byte_word==1)  cmd = REGDEBUG_REG_WRITE_VIRT_B;
           else if( reg_v_p==1&&byte_word==2)  cmd = REGDEBUG_REG_WRITE_VIRT_W;
           else if( reg_v_p==2&&byte_word==1)  cmd = REGDEBUG_REG_WRITE_PHY_B;
           else if( reg_v_p==2&&byte_word==2)  cmd = REGDEBUG_REG_WRITE_PHY_W;           
           else if(user_V_P==1&&byte_word==1)  cmd = REGDEBUG_USER_WRITE_VIRT_B;
           else if(user_V_P==1&&byte_word==2)  cmd = REGDEBUG_USER_WRITE_VIRT_W;           
           else if(user_V_P==2&&byte_word==1)  cmd = REGDEBUG_USER_WRITE_PHY_B;
           else if(user_V_P==2&&byte_word==2)  cmd = REGDEBUG_USER_WRITE_PHY_W;           
                arg_r_w=2;
                break;

      case 'v':
                if(arg_r_w==1&&byte_word==1)   cmd = REGDEBUG_REG_READ_VIRT_B;
           else if(arg_r_w==1&&byte_word==2)   cmd = REGDEBUG_REG_READ_VIRT_W;                
           else if(arg_r_w==2&&byte_word==1)   cmd = REGDEBUG_REG_WRITE_VIRT_B;
           else if(arg_r_w==2&&byte_word==2)   cmd = REGDEBUG_REG_WRITE_VIRT_W;           
                reg_v_p=1;
                break;

      case 'p':
                if(arg_r_w==1&&byte_word==1)   cmd = REGDEBUG_REG_READ_PHY_B;
           else if(arg_r_w==1&&byte_word==2)   cmd = REGDEBUG_REG_READ_PHY_W;                
           else if(arg_r_w==2&&byte_word==1)   cmd = REGDEBUG_REG_WRITE_PHY_B;
           else if(arg_r_w==2&&byte_word==2)   cmd = REGDEBUG_REG_WRITE_PHY_W;           
                reg_v_p=2;
                break;

      case 'V':
                if(arg_r_w==1&&byte_word==1)   cmd = REGDEBUG_USER_READ_VIRT_B;
           else if(arg_r_w==1&&byte_word==2)   cmd = REGDEBUG_USER_READ_VIRT_W;                
           else if(arg_r_w==2&&byte_word==1)   cmd = REGDEBUG_USER_WRITE_VIRT_B;
           else if(arg_r_w==2&&byte_word==2)   cmd = REGDEBUG_USER_WRITE_VIRT_W;           
                user_V_P=1;
                break;

      case 'P':
                if(arg_r_w==1&&byte_word==1)   cmd = REGDEBUG_USER_READ_PHY_B;
           else if(arg_r_w==1&&byte_word==2)   cmd = REGDEBUG_USER_READ_PHY_W;                
           else if(arg_r_w==2&&byte_word==1)   cmd = REGDEBUG_USER_WRITE_PHY_B;
           else if(arg_r_w==2&&byte_word==2)   cmd = REGDEBUG_USER_WRITE_PHY_W;           
                user_V_P=2;
                break;

      case 'B':
                if(arg_r_w==1&&reg_v_p==1)    cmd = REGDEBUG_REG_READ_VIRT_B;
           else if(arg_r_w==1&&reg_v_p==2)    cmd = REGDEBUG_REG_READ_PHY_B;
                if(arg_r_w==2&&reg_v_p==1)    cmd = REGDEBUG_REG_WRITE_VIRT_B;
           else if(arg_r_w==2&&reg_v_p==2)    cmd = REGDEBUG_REG_WRITE_PHY_B;
           else if(arg_r_w==1&&user_V_P==1)   cmd = REGDEBUG_USER_READ_VIRT_B;
           else if(arg_r_w==1&&user_V_P==2)   cmd = REGDEBUG_USER_READ_PHY_B;
           else if(arg_r_w==2&&user_V_P==1)   cmd = REGDEBUG_USER_WRITE_VIRT_B;
           else if(arg_r_w==2&&user_V_P==2)   cmd = REGDEBUG_USER_WRITE_PHY_B;
                byte_word=1;
                break;

      case 'W':
                if(arg_r_w==1&&reg_v_p==1)    cmd = REGDEBUG_REG_READ_VIRT_W;
           else if(arg_r_w==1&&reg_v_p==2)    cmd = REGDEBUG_REG_READ_PHY_W;
                if(arg_r_w==2&&reg_v_p==1)    cmd = REGDEBUG_REG_WRITE_VIRT_W;
           else if(arg_r_w==2&&reg_v_p==2)    cmd = REGDEBUG_REG_WRITE_PHY_W;
           else if(arg_r_w==1&&user_V_P==1)   cmd = REGDEBUG_USER_READ_VIRT_W;
           else if(arg_r_w==1&&user_V_P==2)   cmd = REGDEBUG_USER_READ_PHY_W;
           else if(arg_r_w==2&&user_V_P==1)   cmd = REGDEBUG_USER_WRITE_VIRT_W;
           else if(arg_r_w==2&&user_V_P==2)   cmd = REGDEBUG_USER_WRITE_PHY_W;
                byte_word=2;
                break;

      case 'a':
                if (optarg){
                  args.target_addr = strtoul(optarg,(char **)NULL,0);
                  optarg = NULL;
                }
                break;
      case 'n':
                if (optarg){
                  args.register_value = strtoul(optarg,(char **)NULL,0);
                  optarg = NULL;
                }
                break;
      case 'h':
                printf("\th -- Show this simple help\n");
                printf("\tr -- Read target address value\n");
                printf("\tw -- Write value to target address\n");
                printf("\tv -- IO region virtual address\n");
                printf("\tp -- IO region physical address\n");
                printf("\tV -- User mode virtual address\n");
                printf("\tP -- User mode physical address\n");
                printf("\tB -- Unit:Byte :  8bits\n");
                printf("\tW -- Unit:Word : 32bits\n");
                printf("\ta -- Target address\n");
                printf("\tn -- Value\n");
                printf("Example:\n");
                printf("\t./RdebugUI -r -v -W -a 0xeb400000          \
                \n\t==> read register word(32bits) value at virtual address 0xeb400000\n\n");
                printf("\t./RdebugUI -w -P -B -a 0x00055555 -n 0x77  \
                \n\t==> write byte(8bits) value 0x77 to user mode physical address 0x00055555\n");
                exit(1);
                break;                      
      default:                                           
                printf("arguments parse fail \n");
                exit(1);
        }
    }
     if(cmd == REGDEBUG_IODEFAULT){
	printf("\t -- Use  ./RdebugUI -h  -----> to see howto use this program \n");
        return 1;
     }
     if(args.target_addr==0){
	printf("Need assign target address\n");
        return 1;
     }                                                     
     ioctl(fd,cmd,&args);
     return 0;
}

