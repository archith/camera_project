#include <stdio.h>
#include <unistd.h>
#include <libsmbclient.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <kdef.h>
#include <string.h>
#include <smb_bulk_ops.h>
#include <errno.h>
int lock_smb_fd(char* name)
{
	int fd = -1, i;
	fd = open(name, O_CREAT | O_RDWR, 0666);EQ_ERR(fd, -1);
	for(i=0;i<20;i++)
	{
		if( flock(fd, LOCK_EX | LOCK_NB) == 0)
			break;
		sleep(1);
	}
	if(i==20)
	{
		close(fd);
		fd = -1;
	}
errout:
	if(fd != -1)
		close(fd);
	return fd;
}
char smb_username[SMB_USER_LEN+1];
char smb_password[SMB_PASSWD_LEN+1];
static void smb_auth_fn(const char *server, const char *share, char *workgroup, 
	int wgmaxlen, char *username, int unmaxlen, char *password, int pwmaxlen)
{
	
	memset(workgroup, 0, wgmaxlen);
	memset(username, 0, unmaxlen);
	memset(password, 0, pwmaxlen);
	strcpy(username, smb_username);
	strcpy(password, smb_password);
}
int get_smb_server_fd(struct SMB_DS *ds, char* filename)
{
	int ret, fd, i;
	char buf[256+1];
	ret = smbc_init(smb_auth_fn, 0);NE_ERR(ret, 0);
	for(i=0;i<strlen(ds->path);i++)
	{
		if(ds->path[i]=='\\')
			ds->path[i] = '/';
	}
	sprintf(buf, "smb://%s/%s/%s", ds->server, ds->path, filename);
	buf[256]='\0';
	//fprintf(stderr, "%s\n", buf);
	if(ds->user[0]!='\0')
		strcpy(smb_username, ds->user);
	else
		strcpy(smb_username, "dummy");
	strcpy(smb_password, ds->passwd);
	
	//fprintf(stderr, "%s %s\n", smb_username, smb_password);

	fd = smbc_open(buf, O_RDWR | O_CREAT | O_TRUNC, 0666);EQ_ERR(fd, -1);
	return fd;
errout:
        if(fd != -1)
		smbc_close(fd);
	return -1;
}
int get_vlog_fd(char* filename, char** fptr, int* size)
{
	int fd = -1, ret;
	struct stat st;

	fd = open(filename, O_RDONLY);EQ_ERR(fd, -1);
	ret = fstat(fd, &st);EQ_ERR(ret, -1);

	*fptr = mmap(0, st.st_size, PROT_READ, MAP_SHARED, fd, 0);EQ_ERR(*fptr, MAP_FAILED);
	*size = st.st_size;
	return fd;
errout:
	if(*fptr)
		munmap(*fptr, st.st_size);
	if(fd != -1)
		close(fd);
	return -1;	
}
int main(int argc, char* argv[])
{
	int ret = -1, lock_fd = -1, errcode = -1;
	int ifd = -1, ofd = -1, isize = 0;
	struct SMB_DS ds;
	char* ifptr = NULL, *ofilename;

	NE_ERR(argc, 2);

	lock_fd = lock_smb_fd("/var/lock/smbc_put.lock");EQ_ERR(lock_fd, -1);

	ret = SMB_BULK_ReadDS(&ds);EQ_ERR(ret, -1);
	EQ_ERR(ds.enable, 0);
	ofilename = strrchr(argv[1], '/');EQ_ERR(ofilename, NULL);
	ifd = get_vlog_fd(argv[1], &ifptr, &isize);EQ_ERR(ifd, -1);
	ofd = get_smb_server_fd(&ds, ofilename+1);EQ_ERR_ERRCODE(ofd, -1, errno);

	ret = smbc_write(ofd, ifptr, isize);NE_ERR_ERRCODE(ret, isize, errno);
	errcode = 0;
errout:
	if(ifd != -1)
		close(ifd);
	if(ofd != -1)
		smbc_close(ofd);
	return errcode;
}
