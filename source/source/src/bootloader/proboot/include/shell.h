#ifndef __SHELL_H__
#define __SHELL_H__

typedef struct cmd {
	int	(* fn) (int argc, char **arg);
	char	*name;
	char	*args;
} cmd_t;

extern cmd_t __cmd_start, __cmd_end;

#define __fun_used	__attribute__((__unused__))
#define __cmd_init	__attribute__ ((section (".cmd")))
#define command_init(fn, name, args) \
static int fn (int argc, char **arg); \
static cmd_t __cmd_##fn __fun_used __cmd_init = {fn, name, args}

#include <stdio.h>
#define usage(fn) \
printf ("Usage: %s %s\n", __cmd_##fn.name, __cmd_##fn.args)

extern int cmd_ignore;
extern char *fscript, *escript;
void shell (void);

#endif // __SHELL_H_
