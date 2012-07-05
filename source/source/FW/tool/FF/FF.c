#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#define EQ_ERR(a, b) if(a==b) {fprintf(stderr, "%s:LINE=%d, %d == %d\n", __FILE__, __LINE__, (int)a , (int)b);goto errout;}
#define LS_ERR(a, b) if(a<b) {fprintf(stderr, "%s:LINE=%d, %d < %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
#define NE_ERR(a, b) if(a!=b) {fprintf(stderr, "%s:LINE=%d, %d != %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
int main(int argc, char* argv[])
{
	int start_offset, end_offset;
	char *fptr=NULL;
	int fd=-1, ret, i, errcode=0, size;
	struct stat st;
	NE_ERR(argc, 3);
	
	size = strtol(argv[2], NULL, 0);EQ_ERR(size, 0);
	fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, S_IRWXU|S_IRWXG);EQ_ERR(fd, -1);
	ret = lseek(fd, size-1, SEEK_SET);EQ_ERR(ret, -1);
	write(fd, "a", 1); 
	fptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);EQ_ERR(fptr, MAP_FAILED);
	memset(fptr, 0xFF, size);
	errcode = 0;
errout:
	if(errcode == -1)
	{
		fprintf(stderr, "Make a file with all 0xFF value\n");
		fprintf(stderr, "Usage: ./a.out FILENAME SIZE\n");
		fprintf(stderr, "Example: ./a.out FW.bin 0x800000\n");		
	}	
	if(fptr == NULL)
		munmap(fptr, size);
	if(fd != -1)
		close(fd);
	return errcode;
}

