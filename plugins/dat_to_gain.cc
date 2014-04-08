#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include "ndarray/ndarray.h"

static void fill(const char* fname,double* d)
{
  FILE* f = fopen(fname,"r");
  if (!f)
    ;

  ndarray<double,1> a = make_ndarray(d,96);

  for(double* q=a.begin(); q!=a.end(); q++)
    *q = 0;

  double x,y;
  while(fscanf(f,"%lf %lf",&x,&y)==2)
    a[int(x/10)] = y;

  fclose(f);


  printf("\n");
  for(double* q=a.begin(); q!=a.end(); q++)
    printf("%f%c", *q, ((q-a.begin())%10)==9 ? '\n':' ');
}

int main(int argc,char** argv)
{
  int c;
  double pk=222.;
  while ((c = getopt(argc, argv, "?hp:")) != -1) {
    switch (c) {
    case 'p':
      pk = strtod(optarg,0);
      break;
    }
  }
      
  ndarray<double,2> a = make_ndarray<double>(2,96);
  fill("gain_top.dat"   ,&a[0][0]);
  fill("gain_bottom.dat",&a[1][0]);

  ndarray<double,2> g = make_ndarray<double>(960,960);
  for(unsigned i=0; i<g.shape()[0]; i++)
    for(unsigned j=0; j<g.shape()[1]; j++)
      g[i][j] = pk/a[i/480][j/10];

  FILE* f = fopen("out.gain","w");
  for(unsigned i=0; i<g.shape()[0]; i++) {
    for(unsigned j=0; j<g.shape()[1]; j++)
      fprintf(f,"%f ",g[i][j]);
    fprintf(f,"\n");
  }
  fclose(f);

  return 0;
}
