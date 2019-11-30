int printf(char *fmt);

struct fred {
  int x;
  char y;
  long z;
};

struct fred var2;
struct fred *varptr;

int main() {
  long result;

  var2.x= 12;   printf("%d\n", var2.x);
  var2.y= 'c';  printf("%d\n", var2.y);
  var2.z= 4005; printf("%d\n", var2.z);

  result= var2.x + var2.y + var2.z;
  printf("%d\n", result);

  varptr= &var2;
  result= varptr->x + varptr->y + varptr->z;
  printf("%d\n", result);
  return(0);
}
