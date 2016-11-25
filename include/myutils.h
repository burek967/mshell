#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include "siparse.h"

#define MAX_BACKGROUND_PS 512

int run_pipeline(pipeline, int);
void print_bg_cmds(int);
void sigchild_handler(int);
int check_pipeline(pipeline);

#endif /* !_MYUTILS_H_ */
