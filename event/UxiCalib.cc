#include "UxiCalib.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libgen.h>
#include <math.h>

namespace Ami {

};

using namespace Ami;

static const unsigned _option_no_pedestal         = 0x01;
static const unsigned _option_reload_pedestal     = 0x02;
static const unsigned _option_correct_common_mode = 0x04;
static const unsigned _option_correct_gain        = 0x08;
static const unsigned _option_suppress_bad_pixels = 0x10;

unsigned UxiCalib::option_no_pedestal         () { return _option_no_pedestal; }
unsigned UxiCalib::option_reload_pedestal     () { return _option_reload_pedestal; }
unsigned UxiCalib::option_correct_common_mode () { return _option_correct_common_mode; }
unsigned UxiCalib::option_suppress_bad_pixels () { return _option_suppress_bad_pixels; }
unsigned UxiCalib::option_correct_gain        () { return _option_correct_gain; }

ndarray<unsigned,3> UxiCalib::load_array(const Pds::DetInfo& info,
                                         unsigned nz,
                                         unsigned ny,
                                         unsigned nx,
                                         unsigned def_val,
                                         bool* used_default,
                                         const char* onl_prefix,
                                         const char* off_prefix)
{
  //
  //  Load calibration from a file (include offline too)
  //    Always read and write values for each pixel (even when binned)
  //
  ndarray<unsigned,3> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_array(info, nz, ny, nx, def_val,used_default,f);
    fclose(f);
  }
  else {
    a = make_ndarray<unsigned>(nz,ny,nx);
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
    if (used_default) {
      *used_default = true;
    }
  }
  return a;
}

ndarray<unsigned,3> UxiCalib::load_array(const Pds::DetInfo& info,
                                         unsigned nz,
                                         unsigned ny,
                                         unsigned nx,
                                         unsigned def_val,
                                         bool* used_default,
                                         FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines = nz * ny;
  bool size_match = true;
  CalibIO fio(*f);
  ndarray<unsigned,3> a = make_ndarray<unsigned>(nz,ny,nx);

  for(unsigned z=0; z < nz; z++) {
    if (!size_match) break;
    for(unsigned y=0; y < ny; y++) {
      if (!size_match) break;
      if (fio.next_line()) {
        nlines++;
        for(unsigned x=0; x < nx; x++) {
          unsigned val = (unsigned) (fio.getdb() + 0.5);
          if (fio.get_failed()) {
            // Aren't enough cols in the data
            printf("UxiCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                   Pds::DetInfo::name(info),
                   x,
                   nx);
            size_match = false;
            break;
          }
          a(z,y,x) = val;
        }
        // test if there are more columns in this row than expected
        fio.getdb();
        if (!fio.get_failed()) {
          printf("UxiCalib[%s] retrieved calib data has more columns than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nx);
          size_match = false;
        }
      } else {
        printf("UxiCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
               Pds::DetInfo::name(info),
               nlines,
               maxlines);
        size_match = false;
      }
    }
  }
  if (fio.next_line()) {
    printf("UxiCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("UxiCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}
