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
        if(c == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
        }
#ifdef DEBUG
	//printparsedline(l);
#endif

        // Run

	for(pipe = l->pipelines; *pipe != NULL; ++pipe){
#ifdef DEBUG
	    puts("Running pipeline");
#endif
	    if(run_pipeline(*pipe) == -1)
		continue;
	}
    }
    return 0;
}
