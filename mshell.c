#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)/sizeof(char)-1)

static char buffer[MAX_LINE_LENGTH+1];

int
main(int argc, char *argv[])
{
    int status;

    while(1) {
        // Print prompt
        WRITES(STDOUT_FILENO, PROMPT_STR);

        // Read line
        status = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH+1);
        if(!status)
            break;
        if(status < 0)
            exit(1);
	if(status == MAX_LINE_LENGTH+1) {
	    WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
	    WRITES(STDERR_FILENO, "\n");
	}	
        buffer[status-1] = '\0';
	
        // Parse
        line* l = parseline(buffer);
        command* c = pickfirstcommand(l);
	if(c == NULL) {
	    WRITES(STDERR_FILENO, SYNTAX_ERROR_STR);
	    WRITES(STDERR_FILENO, "\n");
	}

        // Run
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
