char *str;
int   x;

int main() {
  x= -23; printint(x);
  printint(-10 * -10);

  x= 1; x= ~x; printint(x);

  x= 2 > 5; printint(x);
  x= !x; printint(x);
  x= !x; printint(x);

  x= 13; if (x) { printint(13); }
  x= 0; if (!x) { printint(14); }

  for (str= "Hello world\n"; *str; str++) {
    printchar(*str);
  }

  return(0);
}
