void main()
{
  int i;
  for (i= 1; i <= 10; i= i + 1) {
    printint(i);
  }
}

void fred()
{
  int a; int b;
  a= 12; b= 3 * a;
  if (a >= b) { printint(2 * b - a); }
}
