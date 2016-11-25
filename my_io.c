#include "my_io.h"

#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "config.h"

/* Input buffer representation */
static struct line_buffer {
    char line[MAX_LINE_LENGTH+1]; /* data                                     */
    char *end;                    /* end of data or NULL by default           */
    char *pos;                    /* first unprocessed character              */
} prim_buf, snd_buf;

static ssize_t read_line(struct line_buffer *);
static ssize_t read_line_if_neccesary(struct line_buffer *);
static int skip_to_end(struct line_buffer *);
static void append_to_line(struct line_buffer *, const char *, size_t);
static char * find_line_end(const struct line_buffer *);

static inline void
append_to_line(struct line_buffer *dst, const char *from, size_t n)
{
    if(n + (dst->end - dst->line) > MAX_LINE_LENGTH) {
        dst->end = dst->line;
    } else {
        memcpy(dst->end, from, n*sizeof(char));
        dst->end += n;
    }
}

static inline char *
find_line_end(const struct line_buffer *buffer)
{
    char *ret = buffer->pos;
    while(ret != buffer->end && *(ret) != '\n')
        ++ret;
    return ret;
}

static inline ssize_t
read_line_if_neccesary(struct line_buffer *buffer)
{
    if(buffer->pos == NULL || buffer->pos == buffer->end)
        return read_line(buffer);
    return buffer->end - buffer->line;
}

static ssize_t
read_line(struct line_buffer *buffer)
{
    ssize_t status = read(STDIN_FILENO, buffer->line, MAX_LINE_LENGTH);
    if(status < 0) {
        if(errno == EINTR)
            return read_line(buffer);
        return status;
    }
    buffer->pos = buffer->line;
    buffer->end = buffer->line + status;
    return status;
}

static int
skip_to_end(struct line_buffer *buffer)
{
    char *x;
    int k;
    if(buffer->pos != NULL) {
        x = find_line_end(buffer);
        if(x != buffer->end) {
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
next_line()
{
    ssize_t l = read_line_if_neccesary(&prim_buf);
    if(l < 0)
        return NULL;
    if(l == 0) {
        prim_buf.line[0] = '\0';
        return prim_buf.line;
    }

    char *ret = find_line_end(&prim_buf);

    if(ret != prim_buf.end) {
        *ret = '\0';
        char *x = prim_buf.pos;
        prim_buf.pos = ret+1;
        return x;
    }

    snd_buf.end = snd_buf.line;
    append_to_line(&snd_buf, prim_buf.pos, ret - prim_buf.pos);
    while(1) {
        if(read_line(&prim_buf) < 0)
            return NULL;
        ret = find_line_end(&prim_buf);
        append_to_line(&snd_buf, prim_buf.pos, ret-prim_buf.pos);
        if(snd_buf.end == snd_buf.line) {
            skip_to_end(&prim_buf);
            return NULL;
        }
        if(ret != prim_buf.end) {
            *(snd_buf.end) = '\0';
            prim_buf.pos = ret+1;
            return snd_buf.line;
        }
    }
}

int
end_of_input()
{
    return prim_buf.end == prim_buf.line;
}
