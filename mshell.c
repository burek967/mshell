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
#include "builtins.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))

//#define MAX_LINE_LENGTH 10

static char buffer[MAX_LINE_LENGTH+1], snd_buffer[MAX_LINE_LENGTH+1];
static char *buf_end = buffer, *buf2_end = snd_buffer;
static char *linepos = NULL;
static struct stat fd_status;

static char *next_line();
static inline char* find_end(char *, char *);
static inline void copy_to_snd_buf(char *, char *);
static int read_line();
static int read_line_if_neccesary();
static void get_builtin(builtin_pair *, char *);

int
main(int argc, char *argv[])
{
    int status, i;
    int print_prompt;
    builtin_pair builtin;
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
        if(nline == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
            continue;
        }	
        if(buf_end == buffer)
            break;

        // Parse
        line* l = parseline(nline);
        command* c = pickfirstcommand(l);
        if(c == NULL) {
            WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
            WRITES(STDERR_FILENO, "\n");
        }

        // Run
        get_builtin(&builtin, *(c->argv));
        if(builtin.fun != NULL){
            if(builtin.fun(c->argv) != 0){
                WRITES(STDERR_FILENO, "Builtin '");
                write(STDERR_FILENO, builtin.name, strlen(builtin.name));
                WRITES(STDERR_FILENO, "' failed.\n");
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

void get_builtin(builtin_pair *pair, char *cmd){
    if(cmd == NULL){
        pair->name = pair->fun = NULL;
        return;
    }
    builtin_pair *i = builtins_table;
    while(i->name){
        if(strcmp(i->name, cmd) == 0){
            pair->name = i->name;
            pair->fun  = i->fun;
            return;
        }
        i++;
    }
    pair->name = pair->fun = NULL;
}

int read_line_if_neccesary(){
    if(linepos == NULL || linepos == buf_end)
        return read_line();
    return buf_end - buffer;
}

int read_line(){
    int status = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH);
    if(status < 0)
        return status;
    linepos = buffer;
    buf_end = buffer + status;
    return status;
}

inline char* find_end(char *start, char *end){
    while(*start != '\n' && start != end)
        ++start;
    return start;
}

inline void copy_to_snd_buf(char *from, char *to){
    if((to-from) + (buf2_end - snd_buffer) > MAX_LINE_LENGTH)
        buf2_end = snd_buffer;
    else {
        memcpy(buf2_end, from, sizeof(char)*(to-from));
        buf2_end += (to-from);
    }
}

char* next_line(){
    int l = read_line_if_neccesary();
    if(l < 0)
        return NULL;
    if(l == 0){
        *buffer = '\0';
        return buffer;
    }

    char *ret = find_end(linepos, buf_end);

    // command ended

    if(ret != buf_end){
        *ret = '\0';
        char *x = linepos;
        linepos = ret+1;
        return x;
    }

    // command didn't end

    buf2_end = snd_buffer;
    copy_to_snd_buf(linepos, ret);
    while(1) {
        if((l = read_line()) < 0)
            return NULL;
        /*if(l == 0){
            linepos = NULL;
            *buf2_end = '\0';
            return snd_buffer;
        }*/
        ret = find_end(linepos, buf_end);
        copy_to_snd_buf(linepos, ret);
        if(buf2_end == snd_buffer){
            linepos = (ret == buf_end ? NULL : ret+1);
            return NULL;
        }
        if(ret != buf_end){
            *buf2_end = '\0';
            linepos = ret+1;
            return snd_buffer;
        }
    }
    return NULL;
}
