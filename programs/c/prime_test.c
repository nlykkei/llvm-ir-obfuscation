#include <math.h>
#include <stdio.h>

int main() {
  printf("Test integers for primality\n");
  int n = 0;
  int flag = 0;
  
  while (1) {
    printf("Input an integer (0 to exit): ");
    scanf("%d", &n);

    if (n == 0) {
      break;
    }
    
    if (n <= 1) {
      printf("You must input an integer greater than 1\n");
      continue;
    }
    
    for (int i = 2; i <= sqrt(n); ++i) {
      if (n % i == 0) {
	printf("%d is not prime\n", n);
	flag = 1;
	break;
      }
    }

    if (!flag) {
      printf("%d is prime!\n", n);
    }
    
    flag = 0;
  }
  return 0;
}
