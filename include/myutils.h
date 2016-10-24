#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include "builtins.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))

void get_builtin(builtin_pair *, const char *);
char * find_line_end(char *start, char *end);

#endif // _MYUTILS_H_
