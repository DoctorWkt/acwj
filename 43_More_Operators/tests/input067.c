int printf(char *fmt);

typedef int FOO;
FOO var1;

struct bar { int x; int y} ;
typedef struct bar BAR;
BAR var2;

int main() {
  var1= 5; printf("%d\n", var1);
  var2.x= 7; var2.y= 10; printf("%d\n", var2.x + var2.y);
  return(0);
}
