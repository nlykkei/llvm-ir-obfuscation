#include <stdio.h>
int sum = 0;

int main() {
  for (int i = 1; i <= 30; ++i) {
    sum++;
    printf("%d: sum=%d\n", i, sum);
  }

  return sum;
}
