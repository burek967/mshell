#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include "siparse.h"

#define MAX_BACKGROUND_PS 512

/*
 * Executes given pipeline, connecting neighboring commands with a pipe and
 * setting proper file redirections. Returns -1 on error.
 */
int run_pipeline(pipeline, int);

/*
 * Prints information about finished background processes. If program is not
 * running in a charater device, just clears proper structure.
 */
void print_bg_cmds(int);

/*
 * Function to be called on SIGCHILD
 */
void sigchild_handler(int);

#endif /* !_MYUTILS_H_ */
