#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include <fcntl.h>

#include "config.h"
#include "builtins.h"
#include "siparse.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))
#define WRITESTR(fd,x) write(fd, x, strlen(x))

struct line_buffer {
    char line[MAX_LINE_LENGTH+1];
    char *end;
    char *pos;
};

char * next_line();

int end_of_input();

int run_pipeline(pipeline);

#endif /* !_MYUTILS_H_ */
