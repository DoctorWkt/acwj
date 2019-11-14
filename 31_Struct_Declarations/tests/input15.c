int printf(char *fmt);

int main() {
  char  a;
  char *b;
  char  c;

  int   d;
  int  *e;
  int   f;

  a= 18; printf("%d\n", a);
  b= &a; c= *b; printf("%d\n", c);

  d= 12; printf("%d\n", d);
  e= &d; f= *e; printf("%d\n", f);
  return(0);
}
