#ifndef _IPC_TOOLS_H_
#define _IPC_TOOLS_H_

int unix_DS_client(char* data, int data_len, char* path);
int unix_DS_server(char* path);

#endif
