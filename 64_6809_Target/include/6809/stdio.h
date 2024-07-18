#ifndef _STDIO_H_
# define _STDIO_H_
#include <stddef.h>

#ifndef NULL
# define NULL (void *)0
#endif

#ifndef EOF
# define EOF (-1)
#endif

#define __MODE_ERR      0x200   /* Error status */

struct __stdio_file {
  char   *bufpos;        /* the next byte to write to or read from */
  char   *bufread;       /* the end of data returned by last read() */
  char   *bufwrite;      /* highest address writable by macro */
  char   *bufstart;      /* the start of the buffer */
  char   *bufend;        /* the end of the buffer; ie the byte after
                            the last malloc()ed byte */
  int    fd;             /* the file descriptor associated with the stream */
  int    mode;
  char   unbuf   ;       /* The buffer for 'unbuffered' streams */
  char   unbuf1  ;
  char   unbuf2  ;
  char   unbuf3  ;
  char   unbuf4  ;
  char   unbuf5  ;
  char   unbuf6  ;
  char   unbuf7  ;
  struct __stdio_file * next;
};

typedef struct __stdio_file FILE;

#define ferror(fp)	(((fp)->mode&__MODE_ERR) != 0)
#define getc(stream)    fgetc(stream)

FILE *__fopen(char *__path, int __fd, FILE * __stream, char *__mode);
#define fopen(__file, __mode)         __fopen((__file), -1, (FILE*)0, (__mode))
#define freopen(__file, __mode, __fp) __fopen((__file), -1, (__fp), (__mode))
size_t fread(void *ptr, size_t size, size_t nmemb, FILE *stream);
size_t fwrite(void *ptr, size_t size, size_t nmemb, FILE *stream);
int fclose(FILE *stream);
int printf(char *format, ...);
int fprintf(FILE *stream, char *format, ...);
int sprintf(char *str, char *format, ...);
int snprintf(char *str, size_t size, char *format, ...);
int fgetc(FILE *stream);
int getc(FILE *stream);
int getchar(void);
int fputc(int c, FILE *stream);
int fputs(char *s, FILE *stream);
int putc(int c, FILE *stream);
int putchar(int c);
int puts(char *s);
FILE *popen(char *command, char *type);
int pclose(FILE *stream);
char *fgets(char *s, int size, FILE *stream);
int rename(char *path, char *newpath);
int fseek(FILE *stream, long offset, int whence);
long ftell(FILE *stream);
FILE *tmpfile(void);

#ifndef SEEK_SET
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

extern FILE stdin[1];
extern FILE stdout[1];
extern FILE stderr[1];

#endif	// _STDIO_H_
