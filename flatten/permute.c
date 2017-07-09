
#include <stdio.h>

void permute(int g_array[], int n, int *m)
{  
  int prev = g_array[0];
  int next = 0;
  
  for (int i = 0; i < n-1; ++i) {
    next = g_array[i+1];
    g_array[i+1] = prev;
    prev = next;
  }

  g_array[0] = prev;

  *m = (*m + 1) % n; 
}

