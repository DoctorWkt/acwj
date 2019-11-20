int printf(char *fmt);

enum fred { apple=1, banana, carrot, pear=10, peach, mango, papaya };
enum jane { aple=1, bnana, crrot, par=10, pech, mago, paaya };

enum fred var1;
enum jane var2;
enum fred var3;

int main() {
  var1= carrot + pear + mango;
  printf("%d\n", var1);
  return(0);
}
