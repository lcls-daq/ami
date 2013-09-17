#include "Calib.hh"

#include <sys/stat.h>
#include <libgen.h>
#include <stdio.h>
#include <string>

FILE* Ami::Calib::fopen_dual(const char *path1, const char * path2, const char *description)
{
  //  Try this to get client cache coherency
  char path[256];
  strcpy(path,path1);
  struct stat64 st;
  stat64(dirname(path),&st);

  const int ErrMsgSize=200;
  char errmsg[ErrMsgSize];
  FILE *f = fopen(path1, "r");

  if (f) {
    printf("Loaded %s from %s\n", description, path1);
  } else {
    snprintf(errmsg, ErrMsgSize, "fopen: Failed to load %s from %s", description, path1);
    perror(errmsg);

    strcpy(path,path2);
    stat64(dirname(path),&st);
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

