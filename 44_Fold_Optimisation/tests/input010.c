int printf(char *fmt);

void main()
{
  int i; char j;

  j= 20; printf("%d\n", j);
  i= 10; printf("%d\n", i);

  for (i= 1;   i <= 5; i= i + 1) { printf("%d\n", i); }
  for (j= 253; j != 2; j= j + 1) { printf("%d\n", j); }
}
