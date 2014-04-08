#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "ndarray/ndarray.h"

static void fill(const char* fname,ndarray<unsigned,2>& a) 
{
  FILE* f = fopen(fname,"r");
  if (!f)
    ;

  //  skip the header line
  char* p = new char[1024];
  size_t l=1024;
  getline(&p,&l,f);

  for(unsigned row=0; row<a.shape()[0]; row++)
    for(unsigned col=0; col<a.shape()[1]; col++)
      fscanf(f,"%u",&a[row][col]);
  fclose(f);
}

static void fill(const char* fname,ndarray<double,2>& a) 
{
  FILE* f = fopen(fname,"r");
  if (!f)
    ;

  //  skip the header line
  char* p = new char[1024];
  size_t l=1024;
  getline(&p,&l,f);

  for(unsigned row=0; row<a.shape()[0]; row++)
    for(unsigned col=0; col<a.shape()[1]; col++)
      fscanf(f,"%lf",&a[row][col]);
  fclose(f);
}

int main(int argc,char** argv)
{
  unsigned rows=0, cols=0;
  const char* fname=0;
  const char* gname=0;
  unsigned threshold=0;
  unsigned divisions=1;
  unsigned margin=0;

  int c;
  while ((c = getopt(argc, argv, "r:c:f:g:t:h:D:M:")) != -1) {
    switch (c) {
    case 'r': rows=strtoul(optarg,0,0); break;
    case 'c': cols=strtoul(optarg,0,0); break;
    case 't': threshold=strtoul(optarg,0,0); break;
    case 'f': fname=optarg; break;
    case 'g': gname=optarg; break;
    case 'D': divisions=strtoul(optarg,0,0); break;
    case 'M': margin   =strtoul(optarg,0,0); break;
    case 'h':
    default:
      return 1;
    }
  }

  ndarray<unsigned,2> a = make_ndarray<unsigned>(rows,cols);
  fill(fname,a);

  ndarray<unsigned,2> m = make_ndarray<unsigned>(rows,(cols+31)/32);
  for(unsigned i=1; i<a.shape()[0]-1; i++) {
    unsigned* uv = &a[i-1][0];
    unsigned* cv = &a[i][0];
    unsigned* dv = &a[i+1][0];
    m[i][0]=0;
    m[i][m.shape()[1]-1]=0;
    for(unsigned j=1; j<a.shape()[1]-1; j++) 

//       if (uv[j-1]>threshold && uv[j]>threshold && uv[j+1]>threshold &&
//           cv[j-1]>threshold && cv[j]>threshold && cv[j+1]>threshold &&
//           dv[j-1]>threshold && dv[j]>threshold && dv[j+1]>threshold)

      if (uv[j]>threshold && dv[j]>threshold &&
          cv[j]>threshold && cv[j-1]>threshold && cv[j+1]>threshold)

//       if (cv[j]>threshold)

        m[i][j>>5] |= (1<<(j&0x1f));

      else 
        m[i][j>>5] &= ~(1<<(j&0x1f));
  }

  FILE* f = fopen("out.msk","w");
  for(unsigned i=0; i<m.shape()[0]; i++) {
    for(unsigned j=0; j<m.shape()[1]; j++)
      fprintf(f,"%08x ",m[i][j]);
    fprintf(f,"\n");
  }
  fclose(f);

  if (gname) {
    ndarray<double,2> g = make_ndarray<double>(rows,cols);
    fill(gname,g);

    double y0=0,y1=0,y2=0;
    for(unsigned i=0; i<a.shape()[0]; i++) {
      for(unsigned j=0; j<a.shape()[1]; j++) {
        double   v = g[i][j];
        unsigned n = a[i][j];
        if (n>threshold) {
          v /= double(n);
          y0 += 1.;
          y1 += v;
          y2 += v*v;
          g[i][j] = v;
        }
        else
          g[i][j] = 0;
      }
    }

    double mean = y1/y0;
    printf("  n=%f  mean=%f  rms=%f\n",
           y0, mean, sqrt(y2/y0 - mean*mean));

    int drows = (rows+margin)/divisions;
    int dcols = (cols+margin)/divisions;

    FILE* f = fopen("out.gain","w");
    for(unsigned ii=0; ii<divisions; ii++) 
      for(unsigned i=ii*drows; i<(ii+1)*drows-margin; i++) {
        for(unsigned jj=0; jj<divisions; jj++) {
          for(unsigned j=jj*dcols; j<(jj+1)*dcols-margin; j++) {
            fprintf(f,"%lf ",g[i][j] ? mean/g[i][j] : 1.);
          }
        }
        fprintf(f,"\n");
      }
    fclose(f);
  }

  return 0;
}
