int printf(char *fmt);

char  c;
char *str;

int main() {
  c= '\n'; printf("%d\n", c);

  for (str= "Hello world\n"; *str != 0; str= str + 1) {
    printf("%c", *str);
  }
  return(0);
}
