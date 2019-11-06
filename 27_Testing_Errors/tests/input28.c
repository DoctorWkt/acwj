int param8(int a, int b, int c, int d, int e, int f, int g, int h) {
  printint(a); printint(b); printint(c); printint(d);
  printint(e); printint(f); printint(g); printint(h);
  return(0);
}

int fred(int a, int b, int c) {
  return(a+b+c);
}

int main() {
  int x;
  param8(1, 2, 3, 5, 8, 13, 21, 34);
  x= fred(2, 3, 4); printint(x);
  return(0);
}
