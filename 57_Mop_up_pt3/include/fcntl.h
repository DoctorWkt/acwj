#ifndef _FCNTL_H_
# define _FCNTL_H_

#define O_RDONLY 00
#define O_WRONLY 01
#define O_RDWR   02

int open(char *pathname, int flags);

#endif // _FCNTL_H_
