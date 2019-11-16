int printf(char *fmt);

char *str;
int   x;

int main() {
  x= -23; printf("%d\n", x);
  printf("%d\n", -10 * -10);

  x= 1; x= ~x; printf("%d\n", x);

  x= 2 > 5; printf("%d\n", x);
  x= !x; printf("%d\n", x);
  x= !x; printf("%d\n", x);

  x= 13; if (x) { printf("%d\n", 13); }
  x= 0; if (!x) { printf("%d\n", 14); }

  for (str= "Hello world\n"; *str; str++) {
    printf("%c", *str);
  }

  return(0);
}
