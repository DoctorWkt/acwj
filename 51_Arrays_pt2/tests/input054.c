int printf(char *fmt);

int main()
{
  int i;
  for (i=0; i < 20; i++) {
    printf("Hello world, %d\n", i);
  }
  return(0);
}
