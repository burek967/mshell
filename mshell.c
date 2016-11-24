#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <signal.h>

#include "myutils.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"
#include "my_io.h"

int
main(int argc, char *argv[])
{
    int print_prompt;
    struct stat fd_status;
    struct sigaction act;
    char *nline;
    pipeline *pipe;

    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    if(fstat(STDOUT_FILENO, &fd_status) == -1)
        exit(2);

    print_prompt = S_ISCHR(fd_status.st_mode);

    /* set SIGCHILD handler */
    act.sa_handler = sigchild_handler;
    sigaction(SIGCHLD, &act, NULL);

    /* ignore SIGINT */
    act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &act, NULL);

    while(1) {
	print_bg_cmds(print_prompt);
        if(print_prompt){
            WRITES(STDOUT_FILENO, PROMPT_STR);
        }

        nline = next_line();
        if(nline == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
            continue;
        }   
        if(end_of_input())
            break;
        
        line *l = parseline(nline);
        if(l == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
            continue;
        }
        if(l->pipelines == NULL)
            continue;
        for(pipe = l->pipelines; *pipe != NULL; ++pipe)
            run_pipeline(*pipe, (l->flags == LINBACKGROUND));
    }
    return 0;
}
