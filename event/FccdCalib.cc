#include "FccdCalib.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "pdsdata/fccd/FccdConfigV2.hh"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <libgen.h>

using namespace Ami;

static const unsigned Rows   = Pds::FCCD::FccdConfigV2::Trimmed_Row_Pixels;
static const unsigned Cols   = Pds::FCCD::FccdConfigV2::Trimmed_Column_Pixels;
static const unsigned _option_no_pedestal = 1;
static const unsigned _option_reload_pedestal = 2;

unsigned FccdCalib::option_no_pedestal() { return _option_no_pedestal; }

unsigned FccdCalib::option_reload_pedestal() { return _option_reload_pedestal; }

std::string FccdCalib::save_pedestals(Entry* entry,
                                      bool   lsubtract,
				      bool   prod)
{
  const EntryImage& image = *static_cast<const EntryImage*>(entry);
  const DescImage&  desc  = image.desc();

  double* pedestals = new double[Rows*Cols];
  { double* p = pedestals;
    double  n = double(image.info(EntryImage::Normalization));
    double  o = image.info(EntryImage::Pedestal);
    for(unsigned i=0; i<Rows; i++)
      for(unsigned j=0; j<Cols; j++)
	*p++ = (double(image.content(j,i))-o)/n;
  }

  if (lsubtract) {
    //
    //  Load pedestals
    //

    const int NameSize=128;
    char oname1[NameSize];
    char oname2[NameSize];
    
    sprintf(oname1,"ped.%08x.dat",desc.info().phy());
    sprintf(oname2,"/reg/g/pcds/pds/fccdcalib/ped.%08x.dat",desc.info().phy());
    FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");
    
    if (f) {
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      memset(linep, 0, sz);
      char* pEnd = linep;

      double* p = pedestals;
      for(unsigned i=0; i<Rows; i++) {
	getline(&linep, &sz, f);
	*p++ += strtod(linep,&pEnd);
	for(unsigned j=1; j<Cols; j++)
	  *p++ += strtod(pEnd,&pEnd);
      }
      
      free(linep);
      fclose(f);
    }
  }

  char tbuf[32];
  sprintf(tbuf,"%08x.dat",desc.info().phy());
  std::string oname;
    
  if (prod)
    oname = std::string("/reg/g/pcds/pds/fccdcalib/ped.") + tbuf;
  else
    oname = std::string("ped.") + tbuf;
    
  //  rename current pedestal file
  time_t t = time(0);
  strftime(tbuf,32,"%Y%m%d_%H%M%S",localtime(&t));
  
  std::string nname = oname + "." + tbuf;
  rename(oname.c_str(),nname.c_str());
  
  FILE* fn = fopen(oname.c_str(),"w");
  if (!fn) {
    rename(nname.c_str(),oname.c_str());
    delete[] pedestals;
    return std::string("Unable to write new pedestal file ")+oname;
  }
  else {
    double* p = pedestals;
    for(unsigned i=0; i<Rows; i++) {
      for(unsigned j=0; j<Cols; j++)
	fprintf(fn,"%f ",*p++);
      fprintf(fn,"\n");
    }
    fsync(fileno(fn));
    fclose(fn);

    const int NameSize=128;
    char oname1[NameSize];
    strncpy(oname1,oname.c_str(),NameSize);
    int fd = open(dirname(oname1),O_RDONLY|O_DIRECTORY);
    if (fd>=0) {
      fsync(fd);
      close(fd);
    }
    delete[] pedestals;
    return std::string();
  }
}
