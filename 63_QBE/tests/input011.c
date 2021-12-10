int printf(char *fmt);

int main()
{
  int i; char j; long k;

  i= 10; printf("%d\n", i);
  j= 20; printf("%d\n", j);
  k= 30; printf("%d\n", k);

  for (i= 1;   i <= 5; i= i + 1) { printf("%d\n", i); }
  for (j= 253; j != 4; j= j + 1) { printf("%d\n", j); }
  for (k= 1;   k <= 5; k= k + 1) { printf("%d\n", k); }
  return(i);
}
