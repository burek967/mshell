#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include <ctype.h>
#include <stdlib.h>

#include "siparse.h"
#include "config.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))
#define WRITESTR(fd,x) write(fd, x, strlen(x))

/*
 * Input buffer representation
 */
struct line_buffer {
    char line[MAX_LINE_LENGTH+1]; /* data got from read()                     */
    char *end;                    /* pointer to the position after the last
				     or NULL by default                       */
    char *pos;                    /* first unprocessed character              */
};

/*
 * Returns 0-terminated string containing next line of input or NULL on error.
 */
char * next_line();

/*
 * Returns 0 if last read wasn't empty, i.e., if end of input was reached, and
 * 1 otherwise.
 */
int end_of_input();

/*
 * Executes given pipeline, connecting neighboring commands with a pipe and
 * setting proper file redirections. Returns -1 on error.
 */
int run_pipeline(pipeline, int);

void print_bg_cmds();

void sigchild_handler(int);

#endif /* !_MYUTILS_H_ */
