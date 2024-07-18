int printf(char *fmt, ...);

int fred(int x) {
  return(56);
}

void main() {
  int dummy;
  int result;
  dummy= printf("%d\n", 23);
  result= fred(10);
  dummy= printf("%d\n", result);
}
