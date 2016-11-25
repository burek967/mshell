#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>

#include "my_io.h"
#include "builtins.h"

int echo(char *[]);
int undefined(char *[]);
int lexit(char *[]);
int lls(char *[]);
int lkill(char *[]);
int lcd(char *[]);

builtin_pair builtins_table[]={
    {"exit",    &lexit},
    {"lecho",   &echo},
    {"lcd",     &lcd},
    {"lkill",   &lkill},
    {"lls",     &lls},
    {NULL,NULL}
};

int 
echo(char * argv[])
{
    int i =1;
    if (argv[i]) printf("%s", argv[i++]);
    while (argv[i])
        printf(" %s", argv[i++]);

    printf("\n");
    fflush(stdout);
    return 0;
}

int 
undefined(char * argv[])
{
    fprintf(stderr, "Command %s undefined.\n", argv[0]);
    return BUILTIN_ERROR;
}

int
lexit(char * argv[])
{
    exit(0);
    return 0;
}

int
lls(char * argv[])
{
    DIR *dir = opendir(".");
    if(dir == NULL)
        return BUILTIN_ERROR;
    struct dirent *en;
    while((en = readdir(dir)) != NULL) {
        if(*(en->d_name) == '.')
            continue;
        WRITESTR(STDOUT_FILENO, en->d_name);
        WRITES(STDOUT_FILENO, "\n");
    }
    closedir(dir);
    return 0;
}

int
lkill(char * argv[])
{
    int pid, sig = SIGTERM;
    char *end = NULL;
    if(argv[1] == NULL)
        return BUILTIN_ERROR;
    if(*(argv[1]) == '-' && argv[2] != NULL) {
        sig = strtol(argv[1]+1, &end, 10);
        if(*end != '\0')
            return BUILTIN_ERROR;
        pid = strtol(argv[2], &end, 10);
        if(*end != '\0')
            return BUILTIN_ERROR;
    } else {
        pid = strtol(argv[1], &end, 10);
        if(*end != '\0')
            return BUILTIN_ERROR;
    } 
    if(kill(pid, sig) == -1)
        return BUILTIN_ERROR;
    return 0;
}

int
lcd(char * argv[])
{
    if(argv[1] == NULL) {
        char *home = getenv("HOME");
        if(home == NULL || chdir(home) == -1)
            return BUILTIN_ERROR;
        return 0;
    }
    if(argv[2] != NULL || chdir(argv[1]) == -1)
        return BUILTIN_ERROR;
    return 0;
}
