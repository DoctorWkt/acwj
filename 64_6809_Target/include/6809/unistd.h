#ifndef _UNISTD_H_
# define _UNISTD_H_

void _exit(int status);
int unlink(char *pathname);
int fork(void);
int execvp(char *file, char **argv);
int getopt(int argc, char **argv, char *optstring);
int close(int fd);
int read(int fd, void *buf, int len);
int write(int fd, void *buf, int len);
int link(char *path, char *path2);
int chdir(char *path);
int fchdir(int fd);
int rmdir(char *pathname);
int access(char *pathname, int mode);

extern char *optarg;
extern int optind, opterr, optopt;

#define F_OK	0	/* Test for existence.	*/
#define F_ULOCK	0
#define F_LOCK	1
#define F_TLOCK	2
#define F_TEST	3

#endif	// _UNISTD_H_
