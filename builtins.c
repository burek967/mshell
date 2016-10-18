#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <signal.h>

#include "builtins.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))

int echo(char*[]);
int undefined(char *[]);
int lexit(char *[]);
int lls(char *[]);
int lkill(char *[]);
int lcd(char *[]);

builtin_pair builtins_table[]={
	{"exit",	&lexit},
	{"lecho",	&echo},
	{"lcd",		&lcd},
	{"lkill",	&lkill},
	{"lls",		&lls},
	{NULL,NULL}
};

int 
echo( char * argv[])
{
	int i =1;
	if (argv[i]) printf("%s", argv[i++]);
	while  (argv[i])
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
		return 1;
	struct dirent *en;
	while((en = readdir(dir)) != NULL){
		if(*(en->d_name) == '.')
			continue;
		write(STDOUT_FILENO, en->d_name, strlen(en->d_name));
		write(STDOUT_FILENO, "\n", 1);
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
		return 1;
	if(*(argv[1]) == '-'){
		sig = strtol(argv[1], &end, 10);
		if(end != NULL)
			return 1;
		if(argv[2] == NULL)
			return 1;
		pid = strtol(argv[2], &end, 10);
		if(end != NULL)
			return 1;
		if(kill(pid, sig) == -1)
			return 1;
		return 0;
	}
	pid = strtol(argv[1], &end, 10);
	if(end != NULL)
		return 1;
	if(kill(pid, sig) == -1)
		return 1;
	return 0;
}

int
lcd(char * argv[])
{
	if(argv[1] == NULL){
		char *home = getenv("HOME");
		if(home == NULL)
			return 1;
		if(chdir(home) == -1)
			return 1;
		return 0;
	}
	if(chdir(argv[1]) == -1)
		return 1;
	return 0;
}
