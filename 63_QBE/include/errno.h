#ifndef _ERRNO_H_
# define _ERRNO_H_

int * __errno_location(void);

#define errno (* __errno_location())

#endif // _ERRNO_H_
