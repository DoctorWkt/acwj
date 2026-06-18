#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "const.h"

int Line = 0;
int Putback = '\n';
FILE *Infile = NULL;

static void scanfile()
{
    Token t;
    while (scan(&t))
    {
        printf("token: %s", token_str[t.type_idx]);
        if (t.type_idx == T_INTLIT)
            printf(",value: %d", t.value);
        putchar('\n');
    }
}
int main(int argc, char *argv[])
{
    if (argc != 2){
        puts("usage:scanner input.txt");
        exit(0);
    }
    Infile = fopen(argv[1], "r");
    printf("opend file: %s\n",argv[1]);
    if(Infile == NULL){
        puts("file not open!");
    }
    scanfile();
    return 0;
}
