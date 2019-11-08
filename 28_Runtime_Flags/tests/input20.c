int printf(char *fmt);

int a;
int b[25];

int main() {
  b[3]= 12;
  a= b[3];
  printf("%d\n", a);
  return(0);
}
