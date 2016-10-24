#include "builtins.h"

#include <string.h>
#include <unistd.h>

#include "myutils.h"

void get_builtin(builtin_pair *pair, const char *cmd){
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

char * find_line_end(char *start, char *end){
    while(*start != '\n' && start != end)
        ++start;
    return start;
}
