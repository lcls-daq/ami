#include "GainSwitchCalib.hh"

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
static const unsigned _option_correct_common_mode2= 0x08;
static const unsigned _option_correct_common_mode3= 0x10;
static const unsigned _option_correct_gain        = 0x20;
static const unsigned _option_suppress_bad_pixels = 0x40;

unsigned GainSwitchCalib::option_no_pedestal         () { return _option_no_pedestal; }
unsigned GainSwitchCalib::option_reload_pedestal     () { return _option_reload_pedestal; }
unsigned GainSwitchCalib::option_correct_common_mode () { return _option_correct_common_mode; }
unsigned GainSwitchCalib::option_suppress_bad_pixels () { return _option_suppress_bad_pixels; }
unsigned GainSwitchCalib::option_correct_gain        () { return _option_correct_gain; }

unsigned GainSwitchCalib::normalization_option_mask()
{
  return _option_correct_gain;
}

ndarray<unsigned,2> GainSwitchCalib::load_array(const Pds::DetInfo& info,
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
  ndarray<unsigned,2> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_array(info,ny,nx,def_val,used_default,f);
    fclose(f);
  }
  else {
    a = make_ndarray<unsigned>(ny,nx);
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
    if (used_default) {
      *used_default = true;
    }
  }
  return a;
}

ndarray<unsigned,2> GainSwitchCalib::load_array(const Pds::DetInfo& info,
                                                unsigned ny,
                                                unsigned nx,
                                                unsigned def_val,
                                                bool* used_default,
                                                FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines = ny;
  bool size_match = true;
  CalibIO fio(*f);
  ndarray<unsigned,2> a = make_ndarray<unsigned>(ny,nx);

  for(unsigned y=0; y < ny; y++) {
    if (!size_match) break;
    if (fio.next_line()) {
      nlines++;
      for(unsigned x=0; x < nx; x++) {
        unsigned val = fio.getul();
        if (fio.get_failed()) {
          // Aren't enough cols in the data
          printf("GainSwitchCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                 Pds::DetInfo::name(info),
                 x,
                 nx);
          size_match = false;
          break;
        }
        a(y,x) = val;
      }
      // test if there are more columns in this row than expected
      fio.getul();
      if (!fio.get_failed()) {
        printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
               Pds::DetInfo::name(info),
               nx);
        size_match = false;
      }
    } else {
      printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
             Pds::DetInfo::name(info),
             nlines,
             maxlines);
      size_match = false;
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}

ndarray<unsigned,3> GainSwitchCalib::load_array(const Pds::DetInfo& info,
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
    a = load_array(info,nz,ny,nx,def_val,used_default,f);
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

ndarray<unsigned,3> GainSwitchCalib::load_array(const Pds::DetInfo& info,
                                                unsigned nz,
                                                unsigned ny,
                                                unsigned nx,
                                                unsigned def_val,
                                                bool* used_default,
                                                FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines =  nz * ny;
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
          unsigned val = fio.getul();
          if (fio.get_failed()) {
            // Aren't enough cols in the data
            printf("GainSwitchCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                   Pds::DetInfo::name(info),
                   x,
                   nx);
            size_match = false;
            break;
          }
          a(z,y,x) = val;
        }
        // test if there are more columns in this row than expected
        fio.getul();
        if (!fio.get_failed()) {
          printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nx);
          size_match = false;
        }
      } else {
        printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
               Pds::DetInfo::name(info),
               nlines,
               maxlines);
        size_match = false;
      }
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}

ndarray<unsigned,4> GainSwitchCalib::load_array(const Pds::DetInfo& info,
                                                unsigned nm,
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
  ndarray<unsigned,4> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_array(info,nm,nz,ny,nx,def_val,used_default,f);
    fclose(f);
  }
  else {
    a = make_ndarray<unsigned>(nm,nz,ny,nx);
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
    if (used_default) {
      *used_default = true;
    }
  }
  return a;
}

ndarray<unsigned,4> GainSwitchCalib::load_array(const Pds::DetInfo& info,
                                                unsigned nm,
                                                unsigned nz,
                                                unsigned ny,
                                                unsigned nx,
                                                unsigned def_val,
                                                bool* used_default,
                                                FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines =  nm * nz * ny;
  bool size_match = true;
  CalibIO fio(*f);
  ndarray<unsigned,4> a = make_ndarray<unsigned>(nm,nz,ny,nx);

  for(unsigned m=0; m < nm; m++) {
    if (!size_match) break;
    for(unsigned z=0; z < nz; z++) {
      if (!size_match) break;
      for(unsigned y=0; y < ny; y++) {
        if (!size_match) break;
        if (fio.next_line()) {
          nlines++;
          for(unsigned x=0; x < nx; x++) {
            unsigned val = fio.getul();
            if (fio.get_failed()) {
              // Aren't enough cols in the data
              printf("GainSwitchCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                     Pds::DetInfo::name(info),
                     x,
                     nx);
              size_match = false;
              break;
            }
            a(m,z,y,x) = val;
          }
          // test if there are more columns in this row than expected
          fio.getul();
          if (!fio.get_failed()) {
            printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
                   Pds::DetInfo::name(info),
                   nx);
            size_match = false;
          }
        } else {
          printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nlines,
                 maxlines);
          size_match = false;
        }
      }
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}

ndarray<unsigned,2> GainSwitchCalib::load_array_sum(const Pds::DetInfo& info,
                                                    unsigned nm,
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
  ndarray<unsigned,2> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_array_sum(info,nm,ny,nx,def_val,used_default,f);
    fclose(f);
  }
  else {
    a = make_ndarray<unsigned>(ny,nx);
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
    if (used_default) {
      *used_default = true;
    }
  }
  return a;
}

ndarray<unsigned,2> GainSwitchCalib::load_array_sum(const Pds::DetInfo& info,
                                                    unsigned nm,
                                                    unsigned ny,
                                                    unsigned nx,
                                                    unsigned def_val,
                                                    bool* used_default,
                                                    FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines =  nm * ny;
  bool size_match = true;
  CalibIO fio(*f);
  ndarray<unsigned,2> a = make_ndarray<unsigned>(ny,nx);

  for(unsigned m=0; m < nm; m++) {
    if (!size_match) break;
    for(unsigned y=0; y < ny; y++) {
      if (!size_match) break;
      if (fio.next_line()) {
        nlines++;
        for(unsigned x=0; x < nx; x++) {
          unsigned val = fio.getul();
          if (fio.get_failed()) {
            // Aren't enough cols in the data
            printf("GainSwitchCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                   Pds::DetInfo::name(info),
                   x,
                   nx);
            size_match = false;
            break;
          }
          if (m==0)
            a(y,x) = val;
          else
            a(y,x) += val;
        }
        // test if there are more columns in this row than expected
        fio.getul();
        if (!fio.get_failed()) {
          printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nx);
          size_match = false;
        }
      } else {
        printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
               Pds::DetInfo::name(info),
               nlines,
               maxlines);
        size_match = false;
      }
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}

ndarray<unsigned,3> GainSwitchCalib::load_array_sum(const Pds::DetInfo& info,
                                                    unsigned nm,
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
    a = load_array_sum(info,nm,nz,ny,nx,def_val,used_default,f);
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

ndarray<unsigned,3> GainSwitchCalib::load_array_sum(const Pds::DetInfo& info,
                                                    unsigned nm,
                                                    unsigned nz,
                                                    unsigned ny,
                                                    unsigned nx,
                                                    unsigned def_val,
                                                    bool* used_default,
                                                    FILE* f)
{
  unsigned nlines = 0;
  unsigned maxlines =  nm * nz * ny;
  bool size_match = true;
  CalibIO fio(*f);
  ndarray<unsigned,3> a = make_ndarray<unsigned>(nz,ny,nx);

  for(unsigned m=0; m < nm; m++) {
    if (!size_match) break;
    for(unsigned z=0; z < nz; z++) {
      if (!size_match) break;
      for(unsigned y=0; y < ny; y++) {
        if (!size_match) break;
        if (fio.next_line()) {
          nlines++;
          for(unsigned x=0; x < nx; x++) {
            unsigned val = fio.getul();
            if (fio.get_failed()) {
              // Aren't enough cols in the data
              printf("GainSwitchCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
                     Pds::DetInfo::name(info),
                     x,
                     nx);
              size_match = false;
              break;
            }
            if (m==0)
              a(z,y,x) = val;
            else
              a(z,y,x) += val;
          }
          // test if there are more columns in this row than expected
          fio.getul();
          if (!fio.get_failed()) {
            printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
                   Pds::DetInfo::name(info),
                   nx);
            size_match = false;
          }
        } else {
          printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nlines,
                 maxlines);
          size_match = false;
        }
      }
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(unsigned* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}

ndarray<double,3> GainSwitchCalib::load_multi_array(const Pds::DetInfo& info,
                                                    unsigned nm,
                                                    unsigned ny,
                                                    unsigned nx,
                                                    double def_val,
                                                    bool* used_default,
                                                    const char* onl_prefix,
                                                    const char* off_prefix,
                                                    bool expand)
{
  //
  //  Load calibration from a file (include offline too)
  //    Always read and write values for each pixel (even when binned)
  //
  ndarray<double,3> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_multi_array(info,nm,ny,nx,def_val,used_default,f,expand);
    fclose(f);
  }
  else {
    a = make_ndarray<double>(nm,ny,nx);
    for(double* val = a.begin(); val!=a.end(); *val++ = def_val) ;
    if (used_default) {
      *used_default = true;
    }
  }
  return a;
}

ndarray<double,3> GainSwitchCalib::load_multi_array(const Pds::DetInfo& info,
                                                    unsigned nm,
                                                    unsigned ny,
                                                    unsigned nx,
                                                    double def_val,
                                                    bool* used_default,
                                                    FILE* f,
                                                    bool expand)
{
  unsigned nlines = 0;
  unsigned explines = ny;
  unsigned maxlines = nm * ny;
  bool size_match = true;
  bool expanded_data = false;
  CalibIO fio(*f);
  ndarray<double,3> a = make_ndarray<double>(nm,ny,nx);

  for(unsigned m=0; m < nm; m++) {
    if (!size_match || expanded_data) break;
    for(unsigned y=0; y < ny; y++) {
      if (!size_match || expanded_data) break;
      if (fio.next_line()) {
        nlines++;
        for(unsigned x=0; x < nx; x++) {
          double val = fio.getdb();
          if (fio.get_failed()) {
            // Aren't enough cols in the data
            printf("GainSwitchCalib[%s] retrieved calib data has only %d columns less than the expected %d\n",
                   Pds::DetInfo::name(info),
                   x,
                   nx);
            size_match = false;
            break;
          }
          a(m,y,x) = val;
        }
        // test if there are more columns in this row than expected
        fio.getdb();
        if (!fio.get_failed()) {
          printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nx);
          size_match = false;
        }
      } else if (expand && (nlines == explines)) {
        printf("GainSwitchCalib[%s] retrieved calib data has only a single mode - duplicating data for all %u modes\n",
               Pds::DetInfo::name(info),
               nm);
        // Fill all the values from the first mode into the others
        ndarray<double, 2> b(a[0]);
        for(unsigned i=1; i < nm; i++) {
          ndarray<double, 2> c(a[i]);
          if(b.size() != c.size()) {
            printf("GainSwitchCalib[%s] problem duplicating data to other modes since the sizes don't match: %zu vs %zu\n",
                   Pds::DetInfo::name(info),
                   b.size(),
                   c.size());
            size_match = false;
            break;
          }
          for(double *vb = b.begin(), *vc = c.begin(); vb!=b.end(); vb++, vc++)
            *vc = *vb;
        }
        expanded_data = true;
      } else {
        printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
               Pds::DetInfo::name(info),
               nlines,
               maxlines);
        size_match = false;
      }
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(double* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}

ndarray<double,4> GainSwitchCalib::load_multi_array(const Pds::DetInfo& info,
                                                    unsigned nm,
                                                    unsigned nz,
                                                    unsigned ny,
                                                    unsigned nx,
                                                    double def_val,
                                                    bool* used_default,
                                                    const char* onl_prefix,
                                                    const char* off_prefix,
                                                    bool expand)
{
  //
  //  Load calibration from a file (include offline too)
  //    Always read and write values for each pixel (even when binned)
  //
  ndarray<double,4> a;

  FILE* f = Calib::fopen(info, onl_prefix, off_prefix);
  if (f) {
    a = load_multi_array(info,nm,nz,ny,nx,def_val,used_default,f,expand);
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

ndarray<double,4> GainSwitchCalib::load_multi_array(const Pds::DetInfo& info,
                                                    unsigned nm,
                                                    unsigned nz,
                                                    unsigned ny,
                                                    unsigned nx,
                                                    double def_val,
                                                    bool* used_default,
                                                    FILE* f,
                                                    bool expand)
{
  unsigned nlines = 0;
  unsigned explines = nz * ny;
  unsigned maxlines = nm * nz * ny;
  bool size_match = true;
  bool expanded_data = false;
  CalibIO fio(*f);
  ndarray<double,4> a = make_ndarray<double>(nm,nz,ny,nx);

  for(unsigned m=0; m < nm; m++) {
    if (!size_match || expanded_data) break;
    for(unsigned z=0; z < nz; z++) {
      if (!size_match || expanded_data) break;
      for(unsigned y=0; y < ny; y++) {
        if (!size_match || expanded_data) break;
        if (fio.next_line()) {
          nlines++;
          for(unsigned x=0; x < nx; x++) {
            double val = fio.getdb();
            if (fio.get_failed()) {
              // Aren't enough cols in the data
              printf("GainSwitchCalib[%s] retrieved calib data has only %d colums less than the expected %d\n",
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
            printf("GainSwitchCalib[%s] retrieved calib data has more columns than the expected %d\n",
                   Pds::DetInfo::name(info),
                   nx);
            size_match = false;
          }
        } else if (expand && (nlines == explines)) {
          printf("GainSwitchCalib[%s] retrieved calib data has only a single mode - duplicating data for all %u modes\n",
                 Pds::DetInfo::name(info),
                 nm);
          // Fill all the values from the first mode into the others
          ndarray<double, 3> b(a[0]);
          for(unsigned i=1; i < nm; i++) {
            ndarray<double, 3> c(a[i]);
            if(b.size() != c.size()) {
              printf("GainSwitchCalib[%s] problem duplicating data to other modes since the sizes don't match: %zu vs %zu\n",
                     Pds::DetInfo::name(info),
                     b.size(),
                     c.size());
              size_match = false;
              break;
            }
            for(double *vb = b.begin(), *vc = c.begin(); vb!=b.end(); vb++, vc++)
              *vc = *vb;
          }
          expanded_data = true;
        } else {
          printf("GainSwitchCalib[%s] retrieved calib data has only %d rows less than the expected %d\n",
                 Pds::DetInfo::name(info),
                 nlines,
                 maxlines);
          size_match = false;
        }
      }
    }
  }
  if (fio.next_line()) {
    printf("GainSwitchCalib[%s] retrieved calib data has more rows than the expected %d\n",
           Pds::DetInfo::name(info),
           maxlines);
    size_match = false;
  }

  if (!size_match) {
    printf("GainSwitchCalib[%s] retrieved calib data is not of the expected size - clearing data!\n",
           Pds::DetInfo::name(info));
    for(double* val = a.begin(); val!=a.end(); *val++ = def_val) ;
  }

  if (used_default) {
    *used_default = !size_match;
  }

  return a;
}
