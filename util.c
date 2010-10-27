#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <limits.h>
#include <pthread.h>

#include "util.h"
#include "fileio.h"


/* Defined in main.c */
extern int logfd;


int max(int count, ...)
{
  va_list ap;
  int i, temp, max = INT_MIN;

  va_start(ap, count);
  for (i = 0; i < count; i++) {
    temp = va_arg(ap, int); 
    if (temp > max) max = temp;
  }
  va_end(ap);

  return max;
}


int min(int count, ...)
{
  va_list ap;
  int i, temp, min = INT_MAX;

  va_start(ap, count);
  for (i = 0; i < count; i++) {
    temp = va_arg(ap, int); 
    if (temp < min) min = temp;
  }
  va_end(ap);

  return min;
}


void fatal_error(const char *msg)
{
  perror(msg);
  exit(EXIT_FAILURE);
}



