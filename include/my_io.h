#ifndef _MY_IO_H_
#define _MY_IO_H_

#define WRITES(fd,x) write(fd, x, sizeof(x)-sizeof(char))
#define WRITESTR(fd,x) write(fd, x, strlen(x))

/* Returns NULL on read() failure or when line is longer than MAX_LINE_LENGTH */
char * next_line();
int end_of_input();

#endif /* !_MY_IO_H_ */
