int fred() {
  return(56);
}

void main() {
  int dummy;
  int result;
  dummy= printint(23);
  result= fred(10);
  dummy= printint(result);
}
