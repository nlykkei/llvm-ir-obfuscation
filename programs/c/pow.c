#include <stdio.h>
#include <stdlib.h>

int power(int, int);

void help()
{
  printf("Compute power\n");
  printf("Usage: pow <base> <exp>\n");
}

int main(int argc, char* argv[])
{
  int base;
  int exp;
  
  if(argc < 3) {
    printf("Arguments required: <base> <exp>\n");
    return -1;
  }

  if((base = strtol(argv[1], NULL, 0)) <= 0) {
    printf("Invalid argument: <base> must be a positive integer\n");
    return -1;
  }

  if((exp = strtol(argv[2], NULL, 0)) < 0) {
    printf("Invalid argument: <exp> must be a nonnegative integer\n");
    return -1;
  }

  int result = power(base, exp);
  
  return result;
}

int power(int base, int exp)
{
  if(exp == 0) {
    return 1;
  } else if (exp % 2 == 0) {
    int result = power(base, exp / 2);
    return result * result;
  } else {
    return base * power(base, exp - 1);
  }
}
