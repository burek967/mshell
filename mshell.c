#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>

#include "config.h"
#include "siparse.h"
#include "utils.h"

int
main(int argc, char *argv[])
{
    /*
       line * ln;
       command *com;

       ln = parseline("ls -las | grep k | wc ; echo abc > f1 ;  cat < f2 ; echo abc >> f3\n");
       printparsedline(ln);
       printf("\n");
       com = pickfirstcommand(ln);
       printcommand(com,1);

       ln = parseline("sleep 3 &");
       printparsedline(ln);
       printf("\n");

       ln = parseline("echo  & abc >> f3\n");
       printparsedline(ln);
       printf("\n");
       com = pickfirstcommand(ln);
       printcommand(com,1);
       */
    int status;
    char buffer[MAX_LINE_LENGTH];

    while(1){
        // Prompt
        write(STDOUT_FILENO, "$ ", 2);

        // Read line
        status = read(STDIN_FILENO, buffer, MAX_LINE_LENGTH);
        if(!status || status == MAX_LINE_LENGTH)
            break;
        if(status < 0)
            exit(1);
        buffer[status-1] = '\0';

        // Parse
        line* l = parseline(buffer);
        command* c = pickfirstcommand(l);

        // Run
        int k = fork();
        if(k < 0)
            exit(2);
        if(k)
            wait(NULL);
        else {
            execvp(*(c->argv), c->argv);
            switch(errno){
                case EACCES:
                    write(STDOUT_FILENO, *(c->argv), strlen(*(c->argv)));
                    write(STDOUT_FILENO, ": permission denied\n", 20);
                    exit(EXEC_FAILURE);
                case ENOENT:
                    write(STDOUT_FILENO, *(c->argv), strlen(*(c->argv)));
                    write(STDOUT_FILENO, ": no such file or directory\n", 28);
                    exit(EXEC_FAILURE);
                default:
                    write(STDOUT_FILENO, *(c->argv), strlen(*(c->argv)));
                    write(STDOUT_FILENO, ": exec error\n", 13);
                    exit(EXEC_FAILURE);
            }
        }
    }
    write(STDOUT_FILENO, "\n", 1);
}
