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

static char *next_line();

int
main(int argc, char *argv[])
{
    int print_prompt;
    builtin_pair builtin;
    struct stat fd_status;
    char* nline;

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
        nline = next_line();
        if(nline == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
            continue;
        }	
        if(prim_buf.end == prim_buf.line)
            break;

        // Parse
        line *l = parseline(nline);
        command *c = pickfirstcommand(l);
        if(c == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
        }

        // Run
        get_builtin(&builtin, *(c->argv));
        if(builtin.fun != NULL){
            if(builtin.fun(c->argv) != 0){
                WRITES(STDERR_FILENO, "Builtin ");
                write(STDERR_FILENO, builtin.name, strlen(builtin.name));
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
            write(STDERR_FILENO, *(c->argv), strlen(*(c->argv)));
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

/* 
 * Return 0-terminated string containing next line for parse. If the line is too
 * long or read errors occured, return NULL.
 */

char *
next_line()
{
    int l = read_line_if_neccesary(&prim_buf);
    if(l < 0)
        return NULL;
    if(l == 0){
        *(prim_buf.line) = '\0';
        return prim_buf.line;
    }

    char *ret = find_line_end(&prim_buf);

    if(ret != prim_buf.end){
        *ret = '\0';
        char *x = prim_buf.pos;
        prim_buf.pos = ret+1;
        return x;
    }

    snd_buf.end = snd_buf.line;
    append_to_line(&snd_buf, prim_buf.pos, ret - prim_buf.pos);
    while(1) {
        if(read_line(&prim_buf) < 0)
            return NULL;
        ret = find_line_end(&prim_buf);
	append_to_line(&snd_buf, prim_buf.pos, ret-prim_buf.pos);
        if(snd_buf.end == snd_buf.line){
	    skip_to_end(&prim_buf);
            return NULL;
        }
        if(ret != prim_buf.end){
            *(snd_buf.end) = '\0';
	    prim_buf.pos = ret+1;
            return snd_buf.line;
        }
    }
    return NULL;
}
