int printf(char *fmt, ...);

int fred(int x) {
  return(5);
}

void main() {
  int x;
  x= fred(2);
  printf("%d\n", x);
}
