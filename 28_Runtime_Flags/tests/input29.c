int printf(char *fmt);

int param8(int a, int b, int c, int d, int e, int f, int g, int h);
int fred(int a, int b, int c);
int main();

int param8(int a, int b, int c, int d, int e, int f, int g, int h) {
  printf("%d\n", a); printf("%d\n", b); printf("%d\n", c); printf("%d\n", d);
  printf("%d\n", e); printf("%d\n", f); printf("%d\n", g); printf("%d\n", h);
  return(0);
}

int fred(int a, int b, int c) {
  return(a+b+c);
}

int main() {
  int x;
  param8(1, 2, 3, 5, 8, 13, 21, 34);
  x= fred(2, 3, 4); printf("%d\n", x);
  return(0);
}
