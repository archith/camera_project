#ifndef _FUPLOAD_H_
#define _FUPLOAD_H_ 1

#include <stdlib.h>

/* change this if you are using HTTP upload */
#define WRITE_FILE		1
#define NO_WRITE_FILE	0
#define TLS_USERCERT_FILE	"/tmp/tls_usercert"

/* CGI Environment Variables */
#define CONTENT_TYPE getenv("CONTENT_TYPE")
#define CONTENT_LENGTH getenv("CONTENT_LENGTH")
#define HTTP_USER_AGENT getenv("HTTP_USER_AGENT")
typedef struct {
  char *name;
  char *value;
} entrytype;

typedef struct _node {
  entrytype entry;
  struct _node* next;
} node;

typedef struct {
  node* head;
} llist;

int read_file_upload(llist *,unsigned int filesize,int flag,unsigned int *size);
char *cgi_val(llist l,char *name);

char *newstr(char *str);
char *lower_case(char *buffer);
void show_html_page(char *loc);

void list_create(llist *l);
short on_list(llist *l,node *w);
node* list_insafter(llist* l, node* w, entrytype item);
void clear_list(llist* l);
#endif
