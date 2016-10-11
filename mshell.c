#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))

#define MAX_LINE_LENGTH 10

static char buffer[MAX_LINE_LENGTH+1], snd_buffer[MAX_LINE_LENGTH+1];
static char *buf_end = buffer, *buf2_end = snd_buffer;
static char *linepos = NULL;
static struct stat fd_status;

char* next_line();
int read_line();

int
main(int argc, char *argv[])
{
    int status;
    int print_prompt;
    char* nline;

    if(fstat(STDOUT_FILENO, &fd_status) == -1)
        exit(2);

    print_prompt = S_ISCHR(fd_status.st_mode);

    while(1) {
        // Print prompt
        if(print_prompt)
            WRITES(STDOUT_FILENO, PROMPT_STR);

        // Read line
        nline = next_line();
        if(*nline == '\0')
            break;
        if(nline == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
        }	
        puts(nline);
        // Parse
        line* l = parseline(nline);
        command* c = pickfirstcommand(l);
        if(c == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
        }

        // Run
        //printparsedline(l);
        //continue;
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
    WRITES(STDOUT_FILENO, "\n");
    return 0;
}

int read_line(){
    int status = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH+1);
    if(status < 0)
        return status;
    buf_end = buffer + status;
    *buf_end = '\0';
    return status;
}

char* next_line(){
    int l;
    if(linepos == NULL) {
        if((l = read_line()) < 0)
            return NULL;
        if(l == 0){
            *buffer = '\0';
            return buffer;
        }
        linepos = buffer;
    }
    char *ret = linepos;
    while(ret < buf_end && *ret != '\n')
        ++ret;

    // command ended

    if(*ret == '\n'){
        *ret = '\0';
        char *x = linepos;
        linepos = ret+1 >= buf_end ? NULL : ret+1;
        return x;
    }

    // command didn't end

    buf2_end = snd_buffer;
    memcpy(snd_buffer, linepos, sizeof(char)*(ret-linepos));
    buf2_end += ret-linepos;
    while(1) {
        if(buf2_end - snd_buffer > MAX_LINE_LENGTH)
            return NULL;
        if((l = read_line()) < 0)
            return NULL;
        if(l == 0){
            linepos = NULL;
            *buf2_end = '\0';
            return snd_buffer;
        }
        buf_end = buffer + l;
        linepos = buffer;
        for(ret = linepos; ret < buf_end && *ret != '\n'; ++ret);
        if(ret - linepos + (buf2_end - snd_buffer) > MAX_LINE_LENGTH){
            linepos = ret+1 >= buf_end ? NULL : ret+1;
            return NULL;
        }
        memcpy(buf2_end, linepos, sizeof(char)*(ret-linepos));
        buf2_end += ret-linepos;
        if(*ret == '\n'){
            *buf2_end = '\0';
            linepos = ret+1 >= buf_end ? NULL : ret+1;
            return snd_buffer;
        }
    }
    return NULL;
}
