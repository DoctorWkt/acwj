char a; char b; char c;
int  d; int  e; int  f;
long g; long h; long i;


int main() {
  b= 5; c= 7; a= b + c++; printint(a);
  e= 5; f= 7; d= e + f++; printint(d);
  h= 5; i= 7; g= h + i++; printint(g);
  a= b-- + c; printint(a);
  d= e-- + f; printint(d);
  g= h-- + i; printint(g);
  a= ++b + c; printint(a);
  d= ++e + f; printint(d);
  g= ++h + i; printint(g);
  a= b * --c; printint(a);
  d= e * --f; printint(d);
  g= h * --i; printint(g);
  return(0);
}
