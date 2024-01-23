#include "PnccdCalib.hh"

#include "ami/event/FrameCalib.hh"
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
static const unsigned _option_rotate              = 0x18;
static const unsigned _option_correct_gain        = 0x20;

unsigned PnccdCalib::option_no_pedestal        () { return _option_no_pedestal; }
unsigned PnccdCalib::option_reload_pedestal    () { return _option_reload_pedestal; }
unsigned PnccdCalib::option_correct_common_mode() { return _option_correct_common_mode; }
unsigned PnccdCalib::option_correct_gain       () { return _option_correct_gain; }
unsigned PnccdCalib::option_rotate             (Rotation r) { return (r&3)<<3; }
Rotation PnccdCalib::option_rotate             (unsigned r) { return Rotation((r>>3)&3); }

std::string PnccdCalib::save_pedestals(const Entry* entry,
                                       Rotation     r,
                                       bool         prod)
{
  const EntryImage& image = *static_cast<const EntryImage*>(entry);
  const DescImage&  desc  = image.desc();
  unsigned ppb = desc.ppxbin();

  ndarray<double,2> pedestals = make_ndarray<double>(desc.nbinsx()*ppb,
                                                     desc.nbinsy()*ppb);
  { double  n = double(image.info(EntryImage::Normalization)*ppb*ppb);
    double  o = image.info(EntryImage::Pedestal);
    for(unsigned i=0; i<desc.nbinsy(); i++)
      for(unsigned i1=0; i1<ppb; i1++)
        for(unsigned j=0; j<desc.nbinsx(); j++)
          for(unsigned j1=0; j1<ppb; j1++)
            pedestals(i*ppb+i1,j*ppb+j1) = (double(image.content(j,i))-o)/n;
  }

  char tbuf[64];
  sprintf(tbuf,"%08x.dat",desc.info().phy());
  std::string oname;
    
  if (prod)
    oname = std::string("/cds/group/pcds/pds/pnccdcalib/ped.") + tbuf;
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
    fprintf(fn,"# %s\n",oname.c_str());
    unsigned nx = desc.nbinsx()*ppb;
    unsigned ny = desc.nbinsy()*ppb;
    for(unsigned i=0; i<ny; i++) {
      for(unsigned j=0; j<nx; j++) {
        switch(r) {
        case D0:
          fprintf(fn,"%f ",pedestals(i,j));
          break;
        case D90:
          fprintf(fn,"%f ",pedestals(nx-j-1,i));
          break;
        case D180:
          fprintf(fn,"%f ",pedestals(ny-i-1,nx-j-1));
          break;
        case D270:
          fprintf(fn,"%f ",pedestals(j,ny-i-1));
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

ndarray<double,2> PnccdCalib::load_pedestals(const DescImage& d,
                                             Rotation,
                                             bool no_cache) 
{
  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  bool offl_type;
  FILE* f = Calib::fopen(d.info(),"ped","pedestals",no_cache,&offl_type);
  if (f) {
    if (offl_type) {
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      memset(linep, 0, sz);
      char* pEnd = linep;

      ndarray<double,2> pedestals = make_ndarray<double>(1024,1024);
      for(unsigned i=0; i<512; i++) {
        double* p = &pedestals(i,0);
        Ami::Calib::get_line(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
      for(unsigned i=1023; i>=512; i--) {
        double* p = &pedestals(i,511);
        Ami::Calib::get_line(&linep, &sz, f);
        *p-- = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p-- = strtod(pEnd,&pEnd);
      }
      for(unsigned i=1023; i>=512; i--) {
        double* p = &pedestals(i,1023);
        Ami::Calib::get_line(&linep, &sz, f);
        *p-- = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p-- = strtod(pEnd,&pEnd);
      }
      for(unsigned i=0; i<512; i++) {
        double* p = &pedestals(i,512);
        Ami::Calib::get_line(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
      free(linep);
      fclose(f);
      return pedestals;
    }
    else
      return FrameCalib::load_darray(f);
  }
  return ndarray<double,2>();
}

std::string PnccdCalib::save_cmth(const Entry* entry,
                                  Rotation     r,
                                  bool         prod,
                                  double       factor)
{
  const EntryImage& image = *static_cast<const EntryImage*>(entry);
  const DescImage&  desc  = image.desc();
  unsigned ppb = desc.ppxbin();
  unsigned nx  = desc.nbinsx();
  unsigned ny  = desc.nbinsy();

  const ndarray<const uint32_t,2> cont = image.content();
  ndarray<uint32_t,2> a;  // unrotated data
  
  switch(r) {
  case D0:
    a = make_ndarray<uint32_t>(cont.shape()[0]*ppb,cont.shape()[1]*ppb);
    for(unsigned i=0; i<cont.shape()[0]; i++)
      for(unsigned j=0; j<cont.shape()[1]; j++)
        for(unsigned i1=0; i1<ppb; i1++)
          for(unsigned j1=0; j1<ppb; j1++)
            a(i*ppb+i1,j*ppb+j1) = cont(i,j);
    break;
  case D90:
    a = make_ndarray<uint32_t>(cont.shape()[1]*ppb,cont.shape()[0]*ppb);
    for(unsigned i=0; i<cont.shape()[0]; i++)
      for(unsigned j=0; j<cont.shape()[1]; j++)
        for(unsigned i1=0; i1<ppb; i1++)
          for(unsigned j1=0; j1<ppb; j1++)
            a(i*ppb+i1,j*ppb+j1) = cont(nx-j,i);
    break;
  case D180:      
    a = make_ndarray<uint32_t>(cont.shape()[0]*ppb,cont.shape()[1]*ppb);
    for(unsigned i=0; i<cont.shape()[0]; i++)
      for(unsigned j=0; j<cont.shape()[1]; j++)
        for(unsigned i1=0; i1<ppb; i1++)
          for(unsigned j1=0; j1<ppb; j1++)
            a(i*ppb+i1,j*ppb+j1) = cont(ny-i,nx-j);
    break;
  case D270:
    a = make_ndarray<uint32_t>(cont.shape()[1]*ppb,cont.shape()[0]*ppb);
    for(unsigned i=0; i<cont.shape()[0]; i++)
      for(unsigned j=0; j<cont.shape()[1]; j++)
        for(unsigned i1=0; i1<ppb; i1++)
          for(unsigned j1=0; j1<ppb; j1++)
            a(i*ppb+i1,j*ppb+j1) = cont(j,ny-i);
    break;
  default:
    break;
  }

  ndarray<double,2> cmth = make_ndarray<double>(a.shape()[0],8);

  const unsigned nadcch = a.shape()[1]/8;
  for(unsigned i=0; i<a.shape()[0]; i++) {
    for(unsigned ja=0, j=0; ja<8; ja++) {
      double v=0;
      for(unsigned k=0; k<nadcch; k++,j++)
        v += factor*a(i,j);
      cmth(i,ja) = v/double(nadcch);
    }
  }

  char tbuf[64];
  sprintf(tbuf,"%08x.dat",desc.info().phy());
  std::string oname;

  if (prod)
    oname = std::string("/cds/group/pcds/pds/pnccdcalib/cmth.") + tbuf;
  else
    oname = std::string("cmth.") + tbuf;

  //  rename current pedestal file
  time_t t = time(0);
  strftime(tbuf,32,"%Y%m%d_%H%M%S",localtime(&t));

  std::string nname = oname + "." + tbuf;
  rename(oname.c_str(),nname.c_str());

  FILE* fn = fopen(oname.c_str(),"w");
  if (!fn) {
    rename(nname.c_str(),oname.c_str());
    return std::string("Unable to write new common mode file ")+oname;
  }
  else {
    fprintf(fn,"# %s\n",oname.c_str());
    for(unsigned i=0; i<cmth.shape()[0]; i++) {
      for(unsigned j=0; j<cmth.shape()[1]; j++)
        fprintf(fn,"%f ",cmth(i,j));
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

 ndarray<double,2> PnccdCalib::load_cmth(const DescImage& d,
                                           Rotation,
                                           bool no_cache) 
 {
   //
   //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  bool offl_type;
  FILE* f = Calib::fopen(d.info(),"cmth","noise",no_cache,&offl_type);
  if (f) {
    if (!offl_type)
      return FrameCalib::load_darray(f);
  }
  return ndarray<double,2>();
}

ndarray<double,2> PnccdCalib::load_gains(const DescImage& d,
                                         Rotation,
                                         bool no_cache)
{
  //
  //  Load calibration from a file
  //    Always read and write values for each pixel (even when binned)
  //
  bool offl_type;
  FILE* f = Calib::fopen(d.info(),"gain","pixel_gain",no_cache,&offl_type);
  if (f) {
    if (offl_type) {
      size_t sz = 8 * 1024;
      char* linep = (char *)malloc(sz);
      memset(linep, 0, sz);
      char* pEnd = linep;

      ndarray<double,2> gains = make_ndarray<double>(1024,1024);
      for(unsigned i=0; i<512; i++) {
        double* p = &gains(i,0);
        Ami::Calib::get_line(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
      for(unsigned i=1023; i>=512; i--) {
        double* p = &gains(i,511);
        Ami::Calib::get_line(&linep, &sz, f);
        *p-- = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p-- = strtod(pEnd,&pEnd);
      }
      for(unsigned i=1023; i>=512; i--) {
        double* p = &gains(i,1023);
        Ami::Calib::get_line(&linep, &sz, f);
        *p-- = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p-- = strtod(pEnd,&pEnd);
      }
      for(unsigned i=0; i<512; i++) {
        double* p = &gains(i,512);
        Ami::Calib::get_line(&linep, &sz, f);
        *p++ = strtod(linep,&pEnd);
        for(unsigned j=1; j<512; j++)
          *p++ = strtod(pEnd,&pEnd);
      }
      free(linep);
      fclose(f);
      return gains;
    }
    else
      return FrameCalib::load_darray(f);
  }
  return ndarray<double,2>();
}
