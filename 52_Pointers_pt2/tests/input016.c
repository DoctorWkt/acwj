int printf(char *fmt);

int   c;
int   d;
int  *e;
int   f;

int main() {
  c= 12; d=18; printf("%d\n", c);
  e= &c + 1; f= *e; printf("%d\n", f);
  return(0);
}
