#include "const.h"
#include <ctype.h>
static int chrpos(char *s, int c)
{
    char *p;

    p = strchr(s, c);
    return (p ? p - s : -1);
}
static void putback(int c)
{
    Putback = c;
}

static int next()
{
    int c;
    if (Putback)
    {
        c = Putback;
        Putback = 0;
        return c;
    }
    c = fgetc(Infile);
    if (c == '\n')
        Line++;
    return c;
}
static int scanint(int c)
{
    int k, val = 0;

    while ((k = chrpos("0123456789", c)) >= 0)
    {
        val = val * 10 + k;
        c = next();
    }

    putback(c);
    return val;
}

static int skip(void)
{
    int c;

    c = next();
    while (' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c)
    {
        c = next();
    }
    return (c);
}

int scan(Token *t)
{
    int c = skip();
    // printf("read char: %c\n", c);
    switch (c)
    {
    case EOF:
        return (0);
    case '+':
        t->type_idx = T_PLUS;
        break;
    case '-':
        t->type_idx = T_MINUS;
        break;
    case '*':
        t->type_idx = T_STAR;
        break;
    case '/':
        t->type_idx = T_SLASH;
        break;
    default:
        if (isdigit(c))
        {
            t->type_idx = T_INTLIT;
            t->value = scanint(c);
            break;
        }
        printf("Unrecognised character %c on line %d\n", c, Line);
        exit(1);
    }

    return 1;
}
