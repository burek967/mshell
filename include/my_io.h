#ifndef _MY_IO_H_
#define _MY_IO_H_

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))
#define WRITESTR(fd,x) write(fd, x, strlen(x))

/*
 * Returns 0-terminated string containing next line of input or NULL on error.
 */
char * next_line();

/*
 * Returns 0 if last read wasn't empty, i.e., if end of input was reached, and
 * 1 otherwise.
 */
int end_of_input();

#endif /* !_MY_IO_H_ */
