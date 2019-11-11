int printf(char *fmt);

void main()
{
  int x;
  x= 7 < 9;  printf("%d\n", x);
  x= 7 <= 9; printf("%d\n", x);
  x= 7 != 9; printf("%d\n", x);
  x= 7 == 7; printf("%d\n", x);
  x= 7 >= 7; printf("%d\n", x);
  x= 7 <= 7; printf("%d\n", x);
  x= 9 > 7;  printf("%d\n", x);
  x= 9 >= 7; printf("%d\n", x);
  x= 9 != 7; printf("%d\n", x);
}
