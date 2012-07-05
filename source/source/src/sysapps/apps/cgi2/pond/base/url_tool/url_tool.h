#ifndef _URL_TOOL_H
#define _URL_TOOL_H
int url_parser(char* url, char** protocol, char** usr, char** pwd, char** ip, char** port, char** path, char** file);
int url_generator(char* url, char* protocol, char* usr, char* pwd, char* ip, char* port, char* path, char* file);
#endif
