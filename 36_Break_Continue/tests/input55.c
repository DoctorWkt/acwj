int printf(char *fmt);

int main(int argc, char **argv) {
  int i;
  char *argument;
  printf("Hello world\n");

  for (i=0; i < argc; i++) {
    argument= *argv; argv= argv + 1;
    printf("Argument %d is %s\n", i, argument);
  }
  return(0);
}
