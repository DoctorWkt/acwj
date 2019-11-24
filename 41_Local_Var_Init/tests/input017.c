int printf(char *fmt);

int main() {
  char  a;
  char *b;
  int   d;
  int  *e;

  b= &a; *b= 19; printf("%d\n", a);
  e= &d; *e= 12; printf("%d\n", d);
  return(0);
}
