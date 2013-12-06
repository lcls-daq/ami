#include "Calib.hh"

#include <sys/stat.h>
#include <libgen.h>
#include <vector>

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

void Ami::Calib::load_array(ndarray<double,1>& a, unsigned phy, 
                            const char* pfx, const char* dsc)
{
  char path1[128], path2[128];
  sprintf(path1,"%s.%08x.dat",pfx,phy);
  sprintf(path2,"/reg/g/pcds/pds/calib/%s.%08x.dat",pfx,phy);
  FILE* f = Calib::fopen_dual(path1, path2, dsc);

  std::vector<double> va;
  if (f) {
    float v;
    while(!feof(f)) {
      if (fscanf(f,"%f",&v)==1)
        va.push_back(v);
    }
  }
  a = make_ndarray<double>(va.size());
  for(unsigned i=0; i<va.size(); i++)
    a[i] = va[i];
}
