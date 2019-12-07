int printf(char *fmt);

int a;
int b;
int c;
int d;
int e;
int main()
{
  a= 2; b= 4; c= 3; d= 2;
  e= (a+b) * (c+d);
  printf("%d\n", e);
  return(0);
}
