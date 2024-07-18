#ifndef _STRING_H_
# define _STRING_H_

char *strdup(char *s);
char *strchr(char *s, int c);
char *strrchr(char *s, int c);
int strcmp(char *s1, char *s2);
int strncmp(char *s1, char *s2, size_t n);
char *strcpy(char *dst, char *src);
char *strerror(int errnum);
int strlen(char *s);
char *strcat(char *dst, char *src);

#endif	// _STRING_H_
