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

static bool _use_test=false;

void Ami::Calib::use_test(bool v) { _use_test=v; }

bool Ami::Calib::use_test() { return _use_test; }

void Ami::Calib::load_integral_symm(ndarray<double,1>& a, 
				    unsigned phy,
				    const char* pfx, const char* dsc)
{
  char path1[128], path2[128];
  sprintf(path1,"%s.%08x.dat",pfx,phy);
  sprintf(path2,"/reg/g/pcds/pds/calib/%s.%08x.dat",pfx,phy);
  FILE* f = Calib::fopen_dual(path1, path2, dsc);

  std::vector<double> va;
  if (f) {
    float v,x;
    while(!feof(f)) {
      if (fscanf(f,"%f %f",&x,&v)==2)
        va.push_back(v);
    }
  }

  a = make_ndarray<double>(va.size());
  if (va.size()) {
    a[0] = va[0]+va[va.size()-1];
    for(unsigned i=1; i<va.size(); i++)
      a[i] = a[i-1]+va[i]+va[va.size()-i-1];
    double n = a[va.size()-1];
    for(unsigned i=va.size()-1; i>0; i--)
      a[i] = 0.5*(a[i-1]+a[i])/n;
    a[0] *= 0.5;
#if 0
    for(unsigned i=0; i<=va.size(); i++)
      printf("%f%c",a[i], (i%10==9) ? '\n':' ');
    printf("\n");
#endif
  }
}
