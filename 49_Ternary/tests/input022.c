int printf(char *fmt);

char a; char b; char c;
int  d; int  e; int  f;
long g; long h; long i;


int main() {
  b= 5; c= 7; a= b + c++; printf("%d\n", a);
  e= 5; f= 7; d= e + f++; printf("%d\n", d);
  h= 5; i= 7; g= h + i++; printf("%d\n", g);
  a= b-- + c; printf("%d\n", a);
  d= e-- + f; printf("%d\n", d);
  g= h-- + i; printf("%d\n", g);
  a= ++b + c; printf("%d\n", a);
  d= ++e + f; printf("%d\n", d);
  g= ++h + i; printf("%d\n", g);
  a= b * --c; printf("%d\n", a);
  d= e * --f; printf("%d\n", d);
  g= h * --i; printf("%d\n", g);
  return(0);
}
