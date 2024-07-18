#ifndef _UNISTD_H_
# define _UNISTD_H_

void _exit(int status);
int unlink(char *pathname);
int fork(void);
int execvp(char *file, char **argv);
int getopt(int argc, char **argv, char *optstring);
extern char *optarg;
extern int optind, opterr, optopt;

#endif	// _UNISTD_H_
