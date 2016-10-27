#ifndef _MYUTILS_H_
#define _MYUTILS_H_

#include <fcntl.h>

#include "config.h"
#include "builtins.h"
#include "siparse.h"

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))
#define WRITESTR(fd,x) write(fd, x, strlen(x))
#define BUF_EMPTY(buf) ((buf)->end == (buf)->line)

struct line_buffer {
    char line[MAX_LINE_LENGTH+1];
    char *end;
    char *pos;
};

void get_builtin(builtin_pair *, const char *);
int read_line(struct line_buffer *);
int skip_to_end(struct line_buffer *);
char * next_line(struct line_buffer *, struct line_buffer *);
int open_as(char *, int, int);
int run_pipeline(pipeline);

static inline void
append_to_line(struct line_buffer *dst, const char *from, size_t n)
{
    if(n + (dst->end - dst->line) > MAX_LINE_LENGTH){
	dst->end = dst->line;
    } else {
	memcpy(dst->end, from, n*sizeof(char));
	dst->end += n;
    }
}

static inline char *
find_line_end(struct line_buffer *buffer)
{
    char *ret = buffer->pos;
    while(ret != buffer->end && *(ret) != '\n')
        ++ret;
    return ret;
}

static inline int
read_line_if_neccesary(struct line_buffer *buffer)
{
    if(buffer->pos == NULL || buffer->pos == buffer->end)
	return read_line(buffer);
    return buffer->end - buffer->line;
}

#endif /* !_MYUTILS_H_ */
