int main() {
  union foo { int p; };
  int y= (union foo) x;
}
