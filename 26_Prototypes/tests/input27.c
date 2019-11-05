int param8(int a, int b, int c, int d, int e, int f, int g, int h) {
  printint(a); printint(b); printint(c); printint(d);
  printint(e); printint(f); printint(g); printint(h);
  return(0);
}

int param5(int a, int b, int c, int d, int e) {
  printint(a); printint(b); printint(c); printint(d); printint(e);
  return(0);
}

int param2(int a, int b) {
  int c; int d; int e;
  c= 3; d= 4; e= 5;
  printint(a); printint(b); printint(c); printint(d); printint(e);
  return(0);
}

int param0() {
  int a; int b; int c; int d; int e;
  a= 1; b= 2; c= 3; d= 4; e= 5;
  printint(a); printint(b); printint(c); printint(d); printint(e);
  return(0);
}

int main() {
  param8(1,2,3,4,5,6,7,8);
  param5(1,2,3,4,5);
  param2(1,2);
  param0();
  return(0);
}
