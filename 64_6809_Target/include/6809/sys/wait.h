#ifndef _SYS_WAIT.H
#define _SYS_WAIT.H

int waitpid(int pid, int * wstatus, int options);
#define WEXITSTATUS(status)     (((status) & 0xff00) >> 8)
#define WIFEXITED(status)       (((status) & 0xff) == 0)

#endif
