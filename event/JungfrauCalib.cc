#include "JungfrauCalib.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>

//#define DBUG

namespace Ami {

};

using namespace Ami;

static const unsigned _option_no_pedestal         = 0x01;
static const unsigned _option_reload_pedestal     = 0x02;
static const unsigned _option_correct_common_mode = 0x04;
static const unsigned _option_correct_gain        = 0x08;
static const unsigned _option_suppress_bad_pixels = 0x10;
static const unsigned _option_pixel_value_in_kev  = 0x20;

unsigned JungfrauCalib::option_no_pedestal         () { return _option_no_pedestal; }
unsigned JungfrauCalib::option_reload_pedestal     () { return _option_reload_pedestal; }
unsigned JungfrauCalib::option_correct_common_mode () { return _option_correct_common_mode; }
unsigned JungfrauCalib::option_suppress_bad_pixels () { return _option_suppress_bad_pixels; }
unsigned JungfrauCalib::option_correct_gain        () { return _option_correct_gain; }
unsigned JungfrauCalib::option_pixel_value_in_kev  () { return _option_pixel_value_in_kev; }

unsigned JungfrauCalib::normalization_option_mask()
{
  return _option_correct_gain | _option_pixel_value_in_kev;
}

ndarray<double,4> JungfrauCalib::load_multi_array(const Pds::DetInfo& info,
                                               unsigned nm,
                                               unsigned nz,
                                               unsigned ny,
                                               unsigned nx,
                                               double def_val,
                                               bool* used_default,
                                               const char* onl_prefix,
                                               const char* off_prefix)
{
  //
  //  Load calibration from a file (include offline too)
  //    Always read and write values for each pixel (even when binned)
  //
  ndarray<double,4> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_multi_array(info,nm,nz,ny,nx,def_val,used_default,f);
    fclose(f);
  }
  else {
    a = make_ndarray<double>(nm,nz,ny,nx);
    for(double* val = a.begin(); val!=a.end(); *val++ = def_val) ;
    if (used_default) {
      *used_default = true;
    }
  }
  return a;
}

ndarray<double,4> JungfrauCalib::load_multi_array(const Pds::DetInfo& info,
                                               unsigned nm,
                                               unsigned nz,
                                               unsigned ny,
                                               unsigned nx,
                                               double def_val,
                                               bool* used_default,
                                               FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines =  nm * nz * ny;
  bool size_match = true;
  CalibIO fio(*f);
  ndarray<double,4> a = make_ndarray<double>(nm,nz,ny,nx);

  for(unsigned m=0; m < nm; m++) {
    if (!size_match) break;
    for(unsigned z=0; z < nz; z++) {
      if (!size_match) break;
      for(unsigned y=0; y < ny; y++) {
        if (!size_match) break;
        if (fio.next_line()) {
          nlines++;
          for(unsigned x=0; x < nx; x++) {
            double val = fio.getdb();
            if (fio.get_failed()) {
              // Aren't enough cols in the data
              printf("JungfrauCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                     Pds::DetInfo::name(info),
                     x,
                     nx);
              size_match = false;
              break;
            }
            a(m,z,y,x) = val;
          }
          // test if there are more columns in this row than expected
          fio.getdb();
          if (!fio.get_failed()) {
            printf("JungfrauCalib[%s] retrieved calib data has more columns than the expected %d\n",
                   Pds::DetInfo::name(info),
                   nx);
            size_match = false;
          }
        } else {
          printf("JungfrauCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nlines,
                 maxlines);
          size_match = false;
        }
      }
    }
  }
  if (fio.next_line()) {
    printf("JungfrauCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("JungfrauCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(double* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}
