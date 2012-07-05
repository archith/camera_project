#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#define EQ_ERR(a, b) if(a==b) {fprintf(stderr, "%s:LINE=%d, %d == %d\n", __FILE__, __LINE__, (int)a , (int)b);goto errout;}
#define LS_ERR(a, b) if(a<b) {fprintf(stderr, "%s:LINE=%d, %d < %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
#define NE_ERR(a, b) if(a!=b) {fprintf(stderr, "%s:LINE=%d, %d != %d\n", __FILE__, __LINE__, (int)a, (int)b);goto errout;}
int main(int argc, char* argv[])
{
	int start_offset, end_offset, checksum_offset;
	unsigned int *fptr=NULL;
	unsigned int checksum=0;
	int fd=-1, ret, i, errcode=0;
	struct stat st;
	LS_ERR(argc, 4);
	
	start_offset = strtol(argv[2], NULL, 0);
	end_offset = strtol(argv[3], NULL, 0);EQ_ERR(end_offset, 0);
	fd = open(argv[1], O_RDWR);EQ_ERR(fd, -1);
	ret = fstat(fd, &st);EQ_ERR(ret, -1);
	fptr = mmap(0, st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);EQ_ERR(fptr, MAP_FAILED);
	LS_ERR(st.st_size, end_offset);
	LS_ERR(end_offset, start_offset);
	for(i=start_offset/4;i<end_offset/4;i++)
		checksum += fptr[i];
	if(argc == 5)
		checksum_offset = strtol(argv[4], NULL, 0);
	else
		checksum_offset = end_offset;
	fptr[checksum_offset/4]= ~checksum + 1;
	errcode = 0;
errout:
	if(errcode == -1)
	{
		fprintf(stderr, "Usage: ./a.out FILENAME START_OFFSET END_OFFSET [CHECKSUM_OFFSET]\n");
		fprintf(stderr, "Example: ./a.out FW.bin 0x1000 0x2000\n");
		fprintf(stderr, "NOTE: END_ADDRESS will be excluded\n");
	}	
	if(fptr == NULL)
		munmap(fptr, st.st_size);
	if(fd != -1)
		close(fd);
	return errcode;
}

