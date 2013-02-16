#include "Calib.hh"

#include <stdio.h>

FILE* Ami::Calib::fopen_dual(char *path1,char * path2, char *description)
{
  const int ErrMsgSize=200;
  char errmsg[ErrMsgSize];
  FILE *f = fopen(path1, "r");

  if (f) {
    printf("Loaded %s from %s\n", description, path1);
  } else {
    snprintf(errmsg, ErrMsgSize, "fopen: Failed to load %s from %s", description, path1);
    perror(errmsg);
    f = fopen(path2, "r");
    if (f) {
      printf("Loaded %s from %s\n", description, path2);
    } else {
      snprintf(errmsg, ErrMsgSize, "fopen: Failed to load %s from %s", description, path2);
      perror(errmsg);
    }
  }
  return (f);
}

