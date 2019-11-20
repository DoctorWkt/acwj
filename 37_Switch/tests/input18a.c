int printf(char *fmt);

int   a;
int  *b;
char  c;
char *d;

int main()
{
  b= &a; *b= 15; printf("%d\n", a);
  d= &c; *d= 16; printf("%d\n", c);
  return(0);
}
