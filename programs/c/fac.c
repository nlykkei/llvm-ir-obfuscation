#include <stdio.h>
#include <stdlib.h>

int fac(int);

void help()
{
  printf("Compute factorial\n");
  printf("Usage: fac <num>\n");
}

int main(int argc, char* argv[])
{
  int n;
  
  if(argc < 2) {
    printf("Argument required: <num>\n");
    help();
    return -1;
  }

  if((n = strtol(argv[1], NULL, 0)) < 0) {
    printf("Invalid argument: <num> must be a nonnegative integer\n");
    help();
    return -1;
  }

  int result = fac(n);
  
  return result;
}

int fac(int n)
{
  if(n == 0) {
    return 1;
  } else {
    return n * fac(n - 1);
  }
}
