int printf(char *fmt);

int a;
int b;
int c;
int main() {
  a= 42; b= 19;
  printf("%d\n", a & b);
  printf("%d\n", a | b);
  printf("%d\n", a ^ b);
  printf("%d\n", 1 << 3);
  printf("%d\n", 63 >> 3);
  return(0);
}
