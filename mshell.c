#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"

#define STR(s) s, sizeof(s)/sizeof(char)-1
#define ERR(a,b) write(STDERR_FILENO, (a), (b))

int
main(int argc, char *argv[])
{
    int status;
    char buffer[MAX_LINE_LENGTH+1];

    while(1){
        // Prompt
        write(STDOUT_FILENO, STR(PROMPT_STR));

        // Read line
        status = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH+1);
        if(!status)
            break;
        if(status < 0)
            exit(1);
	if(status == MAX_LINE_LENGTH+1) {
	    ERR(STR(SYNTAX_ERROR_STR));
	    ERR(STR("\n"));
	}	
        buffer[status-1] = '\0';
	
        // Parse
        line* l = parseline(buffer);
        command* c = pickfirstcommand(l);
	if(c == NULL) {
	    ERR(STR(SYNTAX_ERROR_STR));
	    ERR(STR("\n"));
	}

        // Run
        int k = fork();
        if(k < 0)
            exit(2);
        if(k)
            wait(NULL);
        else {
            execvp(*(c->argv), c->argv);
	    ERR(*(c->argv), strlen(*(c->argv)));
            switch(errno){
                case EACCES:
                    ERR(STR(": permission denied\n"));
		    break;
                case ENOENT:
                    ERR(STR(": no such file or directory\n"));
		    break;
                default:
                    ERR(STR(": exec error\n"));
		    break;
            }
            exit(EXEC_FAILURE);
        }
    }
    write(STDOUT_FILENO, STR("\n"));
    return 0;
}
