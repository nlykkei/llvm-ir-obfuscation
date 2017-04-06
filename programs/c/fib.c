#include <stdio.h>
#include <stdlib.h>

int fib(int);

void help()
{
  printf("Compute fibonacci\n");
  printf("Usage: fac <num>\n");
}

int main(int argc, char* argv[])
{
  int n;
  
  if(argc < 2) {
    printf("Argument required: <num>\n");
    return -1;
  }

  if((n = strtol(argv[1], NULL, 0)) < 0) {
    printf("Invalid argument: <num> must be a nonegative integer\n");
    return -1;
  }

  int result = fib(n);
  
  return result;
}

int fib(int n)
{
  if(n == 0) {
    return 0;
  } else if(n == 1) {
    return 1;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}
