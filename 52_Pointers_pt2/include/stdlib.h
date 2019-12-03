#ifndef _STDLIB_H_
# define _STDLIB_H_

void exit(int status);
void _Exit(int status);

void *malloc(int size);
void free(void *ptr);
void *calloc(int nmemb, int size);
void *realloc(void *ptr, int size);

#endif	// _STDLIB_H_
