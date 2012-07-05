#ifndef CGI_PARSE_H
#define CGI_PARSE_H

#include <stdlib.h>
#include <string.h>

typedef struct entry
{
	void *data;
	struct entry *next_entry_t;
}entry_t;
typedef struct list_t
{
	entry_t *list_head;
}LIST;
typedef struct cgi_pair_t
{
  char *key;
  char *value;
}cgi_pair;
LIST *cgi_parse(void);
char *cgi_get_value(LIST *head, char *key);
void cgi_pair_remove_all(LIST *List);
int cgi_pair_remove_entry(LIST *head, char *key, char *value);
void cgi_traval_list(LIST *List, void (*function)(void *Data));
int cgi_is_valid_request(void);
#endif

