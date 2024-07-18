#ifndef _SYS_WAIT.H
#define _SYS_WAIT.H

int waitpid(int pid, int * wstatus, int options);

#define	__WEXITSTATUS(status)	(((status) & 0xff00) >> 8)
#define	__WTERMSIG(status)	((status) & 0x7f)
#define	__WSTOPSIG(status)	__WEXITSTATUS(status)
#define	__WIFEXITED(status)	(__WTERMSIG(status) == 0)
#define WEXITSTATUS(status)	__WEXITSTATUS (status)
#define WTERMSIG(status)	__WTERMSIG (status)
#define WIFEXITED(status)	__WIFEXITED (status)

#endif
