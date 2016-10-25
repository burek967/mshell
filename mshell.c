#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

#include "myutils.h"
#include "config.h"
#include "siparse.h"
#include "utils.h"
#include "builtins.h"

static struct line_buffer prim_buf, snd_buf;

int
main(int argc, char *argv[])
{
    int print_prompt;
    builtin_pair builtin;
    struct stat fd_status;
    char *nline;
    pipeline *pipe;

    prim_buf.end = prim_buf.line;
    snd_buf.end = snd_buf.line;
    prim_buf.pos = snd_buf.pos = NULL;

    if(fstat(STDOUT_FILENO, &fd_status) == -1)
        exit(2);

    print_prompt = S_ISCHR(fd_status.st_mode);

    while(1) {
        // Print prompt
        if(print_prompt)
            WRITES(STDOUT_FILENO, PROMPT_STR);

        // Read line
        nline = next_line(&prim_buf, &snd_buf);
        if(nline == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
            continue;
        }	
        if(BUF_EMPTY(&prim_buf))
            break;

        // Parse
        line *l = parseline(nline);
        command *c = pickfirstcommand(l);
	    #ifdef DEBUG
	    printparsedline(l);
	    #endif
        if(c == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
        }

        // Run

	    for(pipe = l->pipelines; *pipe; ++pipe){
	    	if(runpipeline(*pipe) == -1)
	    	    exit(EXEC_FAILURE);
	    }

        get_builtin(&builtin, *(c->argv));
        if(builtin.fun != NULL){
            if(builtin.fun(c->argv) != 0){
                WRITES(STDERR_FILENO, "Builtin ");
                WRITESTR(STDERR_FILENO, builtin.name);
                WRITES(STDERR_FILENO, " error.\n");
            }
            continue;
        }

        int k = fork();
        if(k < 0)
            exit(2);
        if(k)
            wait(NULL);
        else {
            execvp(*(c->argv), c->argv);
            WRITESTR(STDERR_FILENO, *(c->argv));
            switch(errno) {
                case EACCES:
                    WRITES(STDERR_FILENO, ": permission denied\n");
                    break;
                case ENOENT:
                    WRITES(STDERR_FILENO, ": no such file or directory\n");
                    break;
                default:
                    WRITES(STDERR_FILENO, ": exec error\n");
                    break;
            }
            exit(EXEC_FAILURE);
        }
    }
    return 0;
}
