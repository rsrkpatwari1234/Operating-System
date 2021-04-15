#include <stdio.h>
#include <syscall.h>

int main (int argc, char **argv)
{
  int i;
  
  for (i = 0; i < argc; i++)
    printf ("%s\n", argv[i]);
  printf ("\n");
  //printf ("hello\n");
  return EXIT_SUCCESS;
}