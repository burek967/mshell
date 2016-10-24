#include "builtins.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "myutils.h"

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
