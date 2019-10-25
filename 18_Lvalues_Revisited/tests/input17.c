int main() {
  char  a;
  char *b;
  int   d;
  int  *e;

  b= &a; *b= 19; printint(a);
  e= &d; *e= 12; printint(d);
  return(0);
}
