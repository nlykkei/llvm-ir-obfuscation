
int foo(int n) {
  int product = 1;
  for (int i = 1; i <= n; ++i) {
    product *= i;
  }
  return product;
}


int main() {
  int sum = 0;
  for (int i = 1; i < 5; ++i) {
    sum += foo(i);
  }
  return sum;
}



