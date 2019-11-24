int printf(char *fmt);

int fred() {
  return(20);
}

int main() {
  int result;
  printf("%d\n", 10);
  result= fred(15);
  printf("%d\n", result);
  printf("%d\n", fred(15)+10);
  return(0);
}
