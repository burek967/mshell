#include "builtins.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "myutils.h"

static const mode_t def_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

int run_command_bg(command *, int, int[2]);

void
get_builtin(builtin_pair *pair, const char *cmd)
{
    if(cmd == NULL){
	pair->name = NULL;
	pair->fun = NULL;
	return;
    }
    builtin_pair *i;
    for(i = builtins_table; i->name; ++i){
	if(strcmp(i->name, cmd) == 0){
	    pair->name = i->name;
	    pair->fun = i->fun;
	    return;
	}
    }
    pair->name = NULL;
    pair->fun = NULL;
}

int
read_line(struct line_buffer *buffer)
{
    int status = read(STDIN_FILENO, buffer->line, MAX_LINE_LENGTH);
    if(status < 0){
	if(errno == EINTR)
	    return read_line(buffer);
	return status;
    }
    buffer->pos = buffer->line;
    buffer->end = buffer->line + status;
    return status;
}

int
skip_to_end(struct line_buffer *buffer)
{
    char *x;
    int k;
    if(buffer->pos != NULL){
	x = find_line_end(buffer);
	if(x != buffer->end){
	    buffer->pos = x+1;
	    return 0;
	}
    }

    do {
	if((k = read_line(buffer)) <= 0)
	    return k;
	x = find_line_end(buffer);
	if(x != buffer->end) {
	    buffer->pos = x+1;
	    break;
	}
    } while(1);
    return 0;
}

char *
next_line(struct line_buffer *buf1, struct line_buffer *buf2)
{
    int l = read_line_if_neccesary(buf1);
    if(l < 0)
        return NULL;
    if(l == 0){
        *(buf1->line) = '\0';
        return buf1->line;
    }
    
    char *ret = find_line_end(buf1);
    
    if(ret != buf1->end){
        *ret = '\0';
        char *x = buf1->pos;
        buf1->pos = ret+1;
        return x;
    }
    
    buf2->end = buf2->line;
    append_to_line(buf2, buf1->pos, ret - buf1->pos);
    while(1) {
        if(read_line(buf1) < 0)
            return NULL;
        ret = find_line_end(buf1);
	append_to_line(buf2, buf1->pos, ret-buf1->pos);
        if(BUF_EMPTY(buf2)){
	    skip_to_end(buf1);
            return NULL;
        }
        if(ret != buf1->end){
            *(buf2->end) = '\0';
	    buf1->pos = ret+1;
            return buf2->line;
        }
    }
    return NULL;
}

int
mv_fd(int fildes, int fildes2)
{
    if(fildes == fildes2)
	return fildes2;
    if(dup2(fildes,fildes2) == -1)
	return -1;
    if(close(fildes) == -1){
	close(fildes2);
	return -1;
    }
    return 0;
}

int
open_as(char *file, int flags, int fd)
{
    int fil = open(file, flags, def_mode);
    if(fil == -1){
        WRITESTR(STDERR_FILENO, file);
	switch(errno){
        case EACCES:
            WRITES(STDERR_FILENO, ": permission denied\n");
            break;
        case ENOENT:
            WRITES(STDERR_FILENO, ": no such file or directory\n");
	    break;
        }
        return -1;
    }
    if(mv_fd(fil, fd) == -1)
	return -1;
    return 0;
}

/*
 * This should be called from the child process.
 */

int
get_redirections(redirection **redirs)
{
    if(redirs == NULL)
	return 0;
    redirection **redir;
    int flags, fd;
    for(redir = redirs; *redir != NULL; ++redir){
	if(IS_RIN((*redir)->flags)) {
	    flags = O_RDONLY;
	    fd = STDIN_FILENO;
	} else if(IS_RAPPEND((*redir)->flags)){
	    flags = O_WRONLY | O_CREAT | O_APPEND;
	    fd = STDOUT_FILENO;
	} else if(IS_ROUT((*redir)->flags)){
	    flags = O_WRONLY | O_CREAT | O_TRUNC;
	    fd = STDOUT_FILENO;
	} else
	    return -1;
	if(open_as((*redir)->filename, flags, fd) == -1)
	    return -1;
    }
    return 0;
}

/*
 * Check if given pipeline has an empty command.
 */

int
check_pipeline(pipeline p)
{
    if(*p == NULL || *(p+1) == NULL)
	return 0;
    command **c;
    for(c = p; *c != NULL; ++c)
	if((*c)->argv[0] == NULL)
	    return -1;
    return 0;
}

int
run_cmd(command *c)
{
#ifdef DEBUG
    printf("Single command: '%s'\n", *((*p)->argv));
#endif
    builtin_pair builtin;
    get_builtin(&builtin, *(c->argv));
    if(builtin.fun != NULL){
#ifdef DEBUG
	printf("Running builtin %s\n", builtin.name);
#endif
	if(builtin.fun(c->argv) != 0){
	    WRITES(STDERR_FILENO, "Builtin ");
	    WRITESTR(STDERR_FILENO, builtin.name);
	    WRITES(STDERR_FILENO, " error.\n");
	}
	return 0;
    }
    return -1;
}

int
run_pipeline(pipeline p)
{
    if(check_pipeline(p) == -1){
	WRITES(STDERR_FILENO, "Syntax error.\n");
	return -1;
    }
    command **c;
    int in_fd = STDIN_FILENO, proc_cnt = 0;
    int fd[2];
    if(*(p+1) == NULL && run_cmd(*p) != -1)
	return 0;
    for(c = p; *c != NULL; ++c){
	++proc_cnt;
        if(*(c+1) != NULL){
            if(pipe(fd) == -1)
                return -1;
#ifdef DEBUG
	    printf("Pipe: WR=>(%d|%d)=>R\n",fd[1],fd[0]);
#endif
        } else {
	    fd[1] = STDOUT_FILENO;
	    fd[0] = STDIN_FILENO;
	}
	if(run_command_bg(*c, in_fd, fd) == -1)
	    return -1;
	in_fd = fd[0];
    }
    if(in_fd != STDIN_FILENO)
	close(in_fd);
    while(proc_cnt--)
	wait(NULL);
    return 0;
}

int
run_command_bg(command *c, int fd_in, int fd[2])
{
#ifdef DEBUG
    printf("Command %s, IN=%d, OUT=%d\n", *(c->argv), fd_in, fd[1]);
#endif
    int k;
    if((k = fork()) == -1){
#ifdef DEBUG
	puts("Fork failed, cleaning up.");
#endif
	if(fd[0] != STDIN_FILENO)
	    close(fd[0]);
	if(fd[1] != STDOUT_FILENO)
	    close(fd[1]);
	if(fd_in != STDIN_FILENO)
	    close(fd_in);
	return -1;
    }
    if(k == 0){
	if(fd[0] != STDIN_FILENO)
	    close(fd[0]);
        if(mv_fd(fd_in, STDIN_FILENO) == -1)
	    exit(EXEC_FAILURE);
	if(mv_fd(fd[1], STDOUT_FILENO) == -1)
	    exit(EXEC_FAILURE);
	if(get_redirections(c->redirs) == -1)
	    exit(EXEC_FAILURE);
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
    } else {
	if(fd_in != STDIN_FILENO){
	    close(fd_in);
	}
	if(fd[1] != STDOUT_FILENO){
	    close(fd[1]);
	}
    }
    return 0;
}
