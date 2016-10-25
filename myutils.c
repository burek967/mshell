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
