#include "myutils.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include "my_io.h"
#include "config.h"
#include "builtins.h"
#include "siparse.h"

/* Information about processes running currently in the foreground */
static struct {
    pid_t T[MAX_LINE_LENGTH];
    pid_t *end;
} fg_processes;

struct process_info {
    int status;
    pid_t pid;
};

/* Information about closed background processes */
static struct {
    struct process_info T[MAX_BACKGROUND_PS];
    struct process_info *end;
} bg_process_info;

/* Default mode for newly created files (662) */
static const mode_t def_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;

static int fg_empty();
static int fg_remove(pid_t);
static void fg_add(pid_t);
static void bg_add(pid_t, int);
static void get_builtin(builtin_pair *, const char *);
static int run_builtin(command *);
static int run_command(command *, int, int[2], int);
static int get_redirections(redirection **);
static int open_as(const char *, int, int);
static int move_fd(int, int);

void
sigchild_handler(int sig)
{
    pid_t child;
    int stat;
    while((child = waitpid(-1, &stat, WNOHANG)) > 0) {
        if(!fg_remove(child)) {
            bg_add(child, stat);
        }
    }
}

void
print_bg_cmds(int chr)
{
    sigset_t mask, old;
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &old);
    if(chr && bg_process_info.end != NULL) {
        struct process_info *i;
        for(i = bg_process_info.T; i != bg_process_info.end; ++i) {
            if(WIFEXITED(i->status))
                printf("Background process %d terminated. (exited with status %d)\n",
                       i->pid,
                       WEXITSTATUS(i->status));
            else if(WIFSIGNALED(i->status))
                printf("Background process %d terminated. (killed by signal %d)\n",
                       i->pid,
                       WTERMSIG(i->status));
        }
        fflush(stdout);
    }
    bg_process_info.end = bg_process_info.T;
    sigprocmask(SIG_SETMASK, &old, NULL);
}

int
run_pipeline(pipeline p, int bg)
{
    if(*p == NULL)
        return 0;
    command **c;
    int in_fd = STDIN_FILENO;
    int fd[2];
    if(*(p+1) == NULL && run_builtin(*p) == 0)
        return 0;
    sigset_t chld, old;
    sigemptyset(&chld);
    sigaddset(&chld, SIGCHLD);
    sigprocmask(SIG_BLOCK, &chld, &old);
    for(c = p; *c; ++c) {
        if(*(c+1)) {
            if(pipe(fd) == -1)
                goto error;
        } else {
            fd[1] = STDOUT_FILENO;
            fd[0] = STDIN_FILENO;
        }
        if(run_command(*c, in_fd, fd, bg) == -1)
            goto error;
        in_fd = fd[0];
    }
    if(in_fd != STDIN_FILENO)
        close(in_fd);
    if(!bg) {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGINT);
        while(!fg_empty())
            sigsuspend(&mask);
    }
    sigprocmask(SIG_SETMASK, &old, NULL);
    return 0;
error:
    sigprocmask(SIG_SETMASK, &old, NULL);
    return -1;
}

int
check_pipeline(pipeline p)
{
    if(p == NULL)
        return -1;
    if(*p == NULL || *(p+1) == NULL)
        return 0;
    command **c;
    for(c = p; *c != NULL; ++c)
        if((*c)->argv[0] == NULL)
            return -1;
    return 0;
}

static inline int
fg_empty()
{
    return fg_processes.end == NULL || fg_processes.end == fg_processes.T;
}

static int
fg_remove(pid_t pid)
{
    if(fg_empty())
        return 0;
    pid_t *cur;
    for(cur = fg_processes.T; cur != fg_processes.end; ++cur)
        if(*cur == pid)
            break;
    if(cur != fg_processes.end) {
        *cur = *(--fg_processes.end);
        return 1;
    }
    return 0;
}

static void
fg_add(pid_t pid)
{
    if(fg_processes.end == NULL)
        fg_processes.end =fg_processes.T;
    *(fg_processes.end++) = pid;
}

static void
bg_add(pid_t pid, int status)
{
    if(bg_process_info.end == NULL)
        bg_process_info.end = bg_process_info.T;
    if(bg_process_info.end - bg_process_info.T >= MAX_BACKGROUND_PS)
        return;
    bg_process_info.end->pid = pid;
    bg_process_info.end->status = status;
    ++bg_process_info.end;
}

static void
get_builtin(builtin_pair *pair, const char *cmd)
{
    if(pair == NULL)
        return;
    if(cmd == NULL) {
        pair->name = NULL;
        pair->fun = NULL;
        return;
    }
    builtin_pair *i;
    for(i = builtins_table; i->name; ++i) {
        if(strcmp(i->name, cmd) == 0) {
            pair->name = i->name;
            pair->fun = i->fun;
            return;
        }
    }
    pair->name = NULL;
    pair->fun = NULL;
}

static int
run_builtin(command *c)
{
    if(*(c->argv) == NULL)
        return 0;
    builtin_pair builtin;
    get_builtin(&builtin, *(c->argv));
    if(builtin.fun != NULL) {
        if(builtin.fun(c->argv) != 0) {
            WRITES(STDERR_FILENO, "Builtin ");
            WRITESTR(STDERR_FILENO, builtin.name);
            WRITES(STDERR_FILENO, " error.\n");
        }
        return 0;
    }
    return -1;
}

static int
run_command(command *c, int fd_in, int fd[2], int bg)
{
    pid_t k;
    if((k = fork()) == -1)
        exit(3);
    if(k == 0) {
        if(bg)
            setsid();

        /* set default SIGINT and SIGCHLD handler, unblock all signals */
        struct sigaction act;
        sigemptyset(&act.sa_mask);
        act.sa_handler = SIG_DFL;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGCHLD, &act, NULL);
        sigprocmask(SIG_SETMASK, &act.sa_mask, NULL);

        /* set file descriptors */
        if(fd[0] != STDIN_FILENO)
            close(fd[0]);
        if(move_fd(fd_in, STDIN_FILENO) == -1)
            exit(EXEC_FAILURE);
        if(move_fd(fd[1], STDOUT_FILENO) == -1)
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
        if(!bg)
            fg_add(k);
        if(fd_in != STDIN_FILENO)
            close(fd_in);
        if(fd[1] != STDOUT_FILENO)
            close(fd[1]);
    }
    return 0;
}

static int
get_redirections(redirection **redirs)
{
    if(redirs == NULL)
        return 0;
    redirection **redir;
    int flags, fd;
    for(redir = redirs; *redir != NULL; ++redir) {
        if(IS_RIN((*redir)->flags)) {
            flags = O_RDONLY;
            fd = STDIN_FILENO;
        } else if(IS_RAPPEND((*redir)->flags)) {
            flags = O_WRONLY | O_CREAT | O_APPEND;
            fd = STDOUT_FILENO;
        } else if(IS_ROUT((*redir)->flags)) {
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            fd = STDOUT_FILENO;
        } else {
            return -1;
        }
        if(open_as((*redir)->filename, flags, fd) == -1)
            return -1;
    }
    return 0;
}

static int
open_as(const char *file, int flags, int fd)
{
    int fil = open(file, flags, def_mode);
    if(fil == -1) {
        WRITESTR(STDERR_FILENO, file);
        switch(errno) {
        case EACCES:
            WRITES(STDERR_FILENO, ": permission denied\n");
            break;
        case ENOENT:
            WRITES(STDERR_FILENO, ": no such file or directory\n");
            break;
        }
        return -1;
    }
    if(move_fd(fil, fd) == -1)
        return -1;
    return 0;
}

static int
move_fd(int fildes, int fildes2)
{
    if(fildes == fildes2)
        return fildes2;
    if(dup2(fildes,fildes2) == -1)
        return -1;
    if(close(fildes) == -1) {
        close(fildes2);
        return -1;
    }
    return 0;
}
