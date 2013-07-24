#include "PnccdCalib.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>

using namespace Ami;

static const unsigned _option_no_pedestal         = 0x01;
static const unsigned _option_reload_pedestal     = 0x02;
static const unsigned _option_correct_common_mode = 0x04;

unsigned PnccdCalib::option_no_pedestal        () { return _option_no_pedestal; }
unsigned PnccdCalib::option_reload_pedestal    () { return _option_reload_pedestal; }
unsigned PnccdCalib::option_correct_common_mode() { return _option_correct_common_mode; }

std::string PnccdCalib::save_pedestals(Entry* entry,
                                       bool   lsubtract,
                                       bool   prod)
{
  const EntryImage& image = *static_cast<const EntryImage*>(entry);
  const DescImage&  desc  = image.desc();
  unsigned ppb = desc.ppxbin();

  double* pedestals = new double[desc.nbinsx()*desc.nbinsy()*ppb*ppb];
  { double* p = pedestals;
    double  n = double(image.info(EntryImage::Normalization)*ppb*ppb);
    double  o = image.info(EntryImage::Pedestal);
    for(unsigned i=0; i<desc.nbinsy(); i++)
      for(unsigned i1=0; i1<ppb; i1++)
        for(unsigned j=0; j<desc.nbinsx(); j++)
          for(unsigned j1=0; j1<ppb; j1++)
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
    sprintf(oname2,"/reg/g/pcds/pds/pnccdcalib/ped.%08x.dat",desc.info().phy());
    FILE *f = Calib::fopen_dual(oname1, oname2, "pedestals");
    
    if (f) {
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      memset(linep, 0, sz);
      char* pEnd = linep;

      double* p = pedestals;
      for(unsigned i=0; i<desc.nbinsy()*ppb; i++) {
	getline(&linep, &sz, f);
	*p++ += strtod(linep,&pEnd);
	for(unsigned j=1; j<desc.nbinsx()*ppb; j++)
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
    oname = std::string("/reg/g/pcds/pds/pnccdcalib/ped.") + tbuf;
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
    for(unsigned i=0; i<desc.nbinsy()*ppb; i++) {
      for(unsigned j=0; j<desc.nbinsx()*ppb; j++)
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

void PnccdCalib::load_pedestals(EntryImage* correct,
                                bool tform) 
{
  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  const int NameSize=128;
  char oname1[NameSize];
  char oname2[NameSize];
  const DescImage& d = correct->desc();
  sprintf(oname1,"ped.%08x.dat",d.info().phy());
  sprintf(oname2,"/reg/g/pcds/pds/pnccdcalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,"pedestals");
  if (f) {
    size_t sz = 8 * 1024;
    char* linep = (char *)malloc(sz);
    memset(linep, 0, sz);
    char* pEnd = linep;

    const unsigned ppb  = d.ppxbin();
    const unsigned cols = d.nbinsx()*ppb;
    const unsigned rows = d.nbinsy()*ppb;
    const unsigned dsz  = d.nbinsx()*d.nbinsy()*ppb*ppb;
    double* pedestals = new double[dsz];

    double* p = pedestals;
    for(unsigned i=0; i<rows; i++) {
      getline(&linep, &sz, f);
      *p++ += strtod(linep,&pEnd);
      for(unsigned j=1; j<cols; j++)
        *p++ += strtod(pEnd,&pEnd);
    }
    
    free(linep);
    fclose(f);

    for(unsigned iy=0; iy<d.nbinsy(); iy++)
      for(unsigned ix=0; ix<d.nbinsx(); ix++) {
        unsigned v = 0;
        for(unsigned i=0; i<ppb; i++)
          for(unsigned j=0; j<ppb; j++)
            v += pedestals[ix*ppb + j + cols*(iy*ppb + i)];
        //
        //  Rotate the pedestal correction here rather 
        //  than in the PnncdHandler::event method
        //
        if (tform)
          correct->content(v,d.nbinsx()-1-iy,ix);
        else
          correct->content(v,ix,iy);
      }
    delete[] pedestals;
  }
  else
    memset(correct->contents(), 0, d.nbinsx()*d.nbinsy()*sizeof(unsigned)); 
}

