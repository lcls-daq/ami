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
  if (lsubtract)
    return std::string("Select retain pedestal");

  const EntryImage& image = *static_cast<const EntryImage*>(entry);
  const DescImage&  desc  = image.desc();
  unsigned ppb = desc.ppxbin();

  ndarray<double,2> pedestals = make_ndarray<double>(desc.nbinsx()*ppb,
                                                     desc.nbinsy()*ppb);
  { double* p = pedestals.begin();
    double  n = double(image.info(EntryImage::Normalization)*ppb*ppb);
    double  o = image.info(EntryImage::Pedestal);
    for(unsigned i=0; i<desc.nbinsy(); i++)
      for(unsigned i1=0; i1<ppb; i1++)
        for(unsigned j=0; j<desc.nbinsx(); j++)
          for(unsigned j1=0; j1<ppb; j1++)
            *p++ = (double(image.content(j,i))-o)/n;
  }

  char tbuf[64];
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
    return std::string("Unable to write new pedestal file ")+oname;
  }
  else {
    Rotation r = desc.frame(0).r;
    unsigned nx = desc.nbinsx()*ppb;
    unsigned ny = desc.nbinsy()*ppb;
    for(unsigned i=0; i<ny; i++) {
      for(unsigned j=0; j<nx; j++) {
        switch(r) {
        case D0:
          fprintf(fn,"%f ",pedestals[i][j]);
          break;
        case D90:
          fprintf(fn,"%f ",pedestals[nx-j][i]);
          break;
        case D180:
          fprintf(fn,"%f ",pedestals[ny-i][nx-j]);
          break;
        case D270:
          fprintf(fn,"%f ",pedestals[j][ny-i]);
          break;
        default:
          break;
        }
      }
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
    return std::string();
  }
}

void PnccdCalib::load_pedestals(EntryImage* correct,
                                Rotation r,
                                bool no_cache) 
{
  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  const DescImage& d = correct->desc();
  bool offl_type;
  FILE* f = Calib::fopen(d.info(),"ped","pedestals",no_cache,&offl_type);
  if (f) {
    size_t sz = 8 * 1024;
    char* linep = (char *)malloc(sz);
    memset(linep, 0, sz);
    char* pEnd = linep;

    const unsigned ppb  = d.ppxbin();
    ndarray<double,2> pedestals = make_ndarray<double>(1024,1024);

    if (offl_type) {
      for(unsigned i=0; i<512; i++) {
        double* p = &pedestals[i][0];
        getline(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
      for(unsigned i=1023; i>=512; i--) {
        double* p = &pedestals[i][511];
        getline(&linep, &sz, f);
        *p-- = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p-- = strtod(pEnd,&pEnd);
      }
      for(unsigned i=1023; i>=512; i--) {
        double* p = &pedestals[i][1023];
        getline(&linep, &sz, f);
        *p-- = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p-- = strtod(pEnd,&pEnd);
      }
      for(unsigned i=0; i<512; i++) {
        double* p = &pedestals[i][512];
        getline(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
    }
    else {
      for(unsigned i=0; i<1024; i++) {
        double* p = &pedestals[i][0];
        getline(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<1024; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
    }
    
    free(linep);
    fclose(f);

    if (offl_type)
      r = Rotation((r+NPHI-1)%NPHI);

    for(unsigned iy=0; iy<d.nbinsy(); iy++)
      for(unsigned ix=0; ix<d.nbinsx(); ix++) {
        unsigned v = 0;
        for(unsigned i=0; i<ppb; i++)
          for(unsigned j=0; j<ppb; j++)
            v += unsigned(pedestals[iy*ppb + i][ix*ppb + j]);
        //
        //  Rotate the pedestal correction here rather 
        //  than in the PnncdHandler::event method
        //
        switch(r) {
        case D0:
          correct->content(v,ix,iy);
          break;
        case D90:
          correct->content(v,d.nbinsy()-1-iy,ix);
          break;
        case D180:
          correct->content(v,d.nbinsx()-1-ix,d.nbinsy()-1-iy);
          break;
        case D270:
          correct->content(v,iy,d.nbinsx()-1-ix);
          break;
        default:
          break;
        }
      }
  }
  else
    memset(correct->contents(), 0, d.nbinsx()*d.nbinsy()*sizeof(unsigned)); 
}

