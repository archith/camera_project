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

#define CACHETEST_IOC_MAGIC  'K'

#define CACHETEST_QUANTUM  4096
#define CACHETEST_IOCOFFSET  				_IOW(CACHETEST_IOC_MAGIC, 0, quantum)
#define CACHETEST_IOCINVAILD  			_IOW(CACHETEST_IOC_MAGIC, 1, quantum)
#define CACHETEST_IOCINVAILDALIGN  	_IOW(CACHETEST_IOC_MAGIC, 2, quantum)
#define CACHETEST_IOCCLEAN	  			_IOW(CACHETEST_IOC_MAGIC, 3, quantum)
#define CACHETEST_IOCFLUSH	  			_IOW(CACHETEST_IOC_MAGIC, 4, quantum)
#define CACHETEST_IOCFLUSHCLEAN			_IOW(CACHETEST_IOC_MAGIC, 5, quantum)
#define CACHETEST_IOCFLUSHPAGE 			_IOW(CACHETEST_IOC_MAGIC, 6, quantum)
#define CACHETEST_IOCFLUSHBOUNDARY	_IOW(CACHETEST_IOC_MAGIC, 7, quantum)
#define CACHETEST_IOCICACHE					_IOW(CACHETEST_IOC_MAGIC, 8, quantum)

#define MODEMDEVICE "/dev/CacheTS"

int quantum = CACHETEST_QUANTUM;

void
help(void)
{
	printf("\n");
	printf("\t-n  change test memory lengt\n");
	printf("\t-o  change memory offset\n");
	printf("\t-i  Use 'invalidate_dcache_range' function to test D-Cache\n");
	printf("\t-a  Use 'invalidate_dcache_range' function to test D-Cache alignment\n");
	printf("\t-c  Use 'clean_dcache_range'      function to test D-Cache\n");
	printf("\t-f  Use 'flush_dcache_range'      function to test D-Cache\n");
	printf("\t-e  Use 'clean_dcache_entry'      function to test D-Cache\n");
	printf("\t-p  Use 'flush_dcache_page'       function to test D-Cache\n");
	printf("\t-b  Use 'invalidate_dcache_range' function to test D-Cache page boundary\n");
	printf("\t-I  Test I-Cache \n");
	
}	

int
main(int argc, char **argv)
{	

  int fd,ch;
  int test_num = 512;
  int offset=0;
  int cmd = CACHETEST_IOCINVAILD;


//	printf("==>main\n");
	
	optarg = NULL;
  opterr = 0;

  fd = open(MODEMDEVICE, O_RDWR | O_NOCTTY );
	if(fd<0){
		printf("CACHETESTUI open fail \n");
		return 0;
	}

	while ((ch = getopt(argc, argv, "o:iacfn:epbIh")) != -1)
    {
        switch(ch)
        {
        		 case 'o':
            					if (optarg){
                          offset = atoi(optarg);
                      }else{
                      		offset = 0;
                      }
                      cmd = CACHETEST_IOCOFFSET;
                      break;
            case 'i':
            					cmd = CACHETEST_IOCINVAILD;
                      break;
            case 'a':
            					cmd = CACHETEST_IOCINVAILDALIGN;
                      break;
						case 'c':
            					cmd = CACHETEST_IOCCLEAN;
                      break;
						case 'f':
            					cmd = CACHETEST_IOCFLUSH;
                      break;
            case 'n':
            					if (optarg){
                          test_num = atoi(optarg);
                      }
                      break;  
            case 'e':
            					cmd = CACHETEST_IOCFLUSHCLEAN;
                      break;
						case 'p':
            					cmd = CACHETEST_IOCFLUSHPAGE;
                      break;
            case 'b':
            					cmd = CACHETEST_IOCFLUSHBOUNDARY;
                      break;
            case 'I':
            					cmd = CACHETEST_IOCICACHE;
                      break;
            case 'h':          
            					help();
                 			close(fd);
                    	return 0;
											
            default: 
                     exit(2);
        }
    }
    
	if(cmd == CACHETEST_IOCOFFSET){
			ioctl(fd,cmd,&offset);
	}
	else{
  		ioctl(fd,cmd,&test_num);
  }
  
	close(fd);
 
 return 0;
}

