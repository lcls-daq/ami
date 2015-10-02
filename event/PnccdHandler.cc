#include "PnccdHandler.hh"

#include "ami/event/PnccdCalib.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"

#include "psalg/psalg.h"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

//#define DBUG

static const unsigned rows = 1024;
static const unsigned cols = 1024;
static const unsigned rows_segment = 512;
static const unsigned cols_segment = 512;

static const unsigned PixelVMask = 0x3fff;
static const unsigned Offset=1024;
static const unsigned MinCalib=25;

static int PixelsPerBin = 1;
static unsigned ArraySize=rows*cols/(PixelsPerBin*PixelsPerBin);

static const unsigned testx = 1;
static const unsigned testy = 1;

namespace Ami {
  class PixelCalibration {
  public:
    enum { NMips=2 };
    PixelCalibration() { reset(); }
  public:
    void reset() { _sum=0; for(int i=0; i<NMips; i++) { _low[i]=PixelVMask; _high[i]=0; } }
    void add(unsigned v) { 
      for(unsigned i=0; i<NMips; i++) {
	if (v < _low [i]) { _replace(v, _low , i, PixelVMask); return; }
	if (v > _high[i]) { _replace(v, _high, i,          0); return; }
      }
      _sum += v;
    }
    unsigned avg(unsigned n) { int N = n-2*NMips; return (_sum+(N>>1))/N; }
  private:
    void _replace(unsigned v, unsigned* array, unsigned i, unsigned init) 
    {
      if (array[NMips-1]!=init) _sum += array[NMips-1];
      for(unsigned j=NMips-1; j>i; j--)
	array[j] = array[j-1];
      array[i] = v;
    }
  private:
    unsigned _sum;
    unsigned _low[NMips], _high[NMips];
  };
};

using namespace Ami;


PnccdHandler::PnccdHandler(const Pds::DetInfo& info,
			   const FeatureCache& cache) : 
  EventHandler(info, Pds::TypeId::Id_pnCCDframe, Pds::TypeId::Id_pnCCDconfig),
  _cache   (cache),
  _config  (0,0),
  _entry   (0),
  _options (0)
{
  PixelsPerBin = _full_resolution() ? 1 : 2;

  const int ppb = PixelsPerBin;
  ArraySize    = rows*cols/(ppb*ppb);

  _calib   = new PixelCalibration[ArraySize];
}
  
PnccdHandler::~PnccdHandler()
{
  delete[] _calib;
}

#ifdef DBUG
unsigned PnccdHandler::nentries() const { return _entry ? 2 : 0; }
#else
unsigned PnccdHandler::nentries() const { return _entry ? 1 : 0; }
#endif

//const Entry* PnccdHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }
const Entry* PnccdHandler::entry(unsigned i) const { 
  switch(i) {
  case 0: return _entry;
  case 1: return _common;
  default: break;
  }
  return 0;
}

void PnccdHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void PnccdHandler::reset() { _entry = 0; _common = 0; }

void PnccdHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  _config = *reinterpret_cast<const Pds::PNCCD::ConfigV1*>(payload);

  Ami::Rotation r(D0);

  /*
  char oname1[256];
  char oname2[256];
  sprintf(oname1,"rot.%08x.dat",info().phy());
  sprintf(oname2,"/reg/g/pcds/pds/pnccdcalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,"rotation");
  if (f) {
    r = Ami::D0;
    fclose(f);
  }
  else
    r = Ami::D90;
  */

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, (unsigned)0, ChannelID::name(det),
		 cols / PixelsPerBin,
		 rows / PixelsPerBin,
		 PixelsPerBin, PixelsPerBin);
  desc.add_frame(0,0,cols/PixelsPerBin,rows/PixelsPerBin,r);
  _entry = new EntryImage(desc);

  DescImage cdsc(det, (unsigned)1, 
		 (std::string(desc.name())+std::string("-cmn")).c_str(),
		 cols / PixelsPerBin,
		 rows / PixelsPerBin,
		 PixelsPerBin, PixelsPerBin);
  _common = new EntryImage(cdsc);

  _ped  = PnccdCalib::load_pedestals(_entry->desc(),r,false);
  _cmth = PnccdCalib::load_cmth     (_entry->desc(),r,false);
}

void PnccdHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void PnccdHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (!_entry) return;

  if (_entry && _entry->desc().used()) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("PnccdHandler options %x -> %x\n",_options,o);
      _options = o;
    }

    if (_entry->desc().options() & PnccdCalib::option_reload_pedestal()) {
      _ped  = PnccdCalib::load_pedestals(_entry->desc(),_entry->desc().frame(0).r,false);
      _cmth = PnccdCalib::load_cmth     (_entry->desc(),_entry->desc().frame(0).r,false);
      _entry->desc().options( _entry->desc().options()&~PnccdCalib::option_reload_pedestal() );
    }
  }

  const Pds::PNCCD::FramesV1& f = *reinterpret_cast<const Pds::PNCCD::FramesV1*>(payload);
  _fillQuadrant (f.frame(_config,0).data(_config).data(), 0, 0);
  _fillQuadrantR(f.frame(_config,1).data(_config).data(), cols_segment-1, rows-1);
  _fillQuadrantR(f.frame(_config,2).data(_config).data(), cols-1, rows-1);
  _fillQuadrant (f.frame(_config,3).data(_config).data(), cols_segment, 0);

  double n = double(PixelsPerBin*PixelsPerBin);
  _entry->info(double(Offset)*n,EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);
#ifdef DBUG
  _common->info(0,EntryImage::Pedestal);
  _common->info(1,EntryImage::Normalization);
  _common->valid(t);
#endif
}

void PnccdHandler::_damaged() { if (_entry) _entry->invalid(); }

void PnccdHandler::_begin_calib() 
{
}

void PnccdHandler::_end_calib()
{
}

void PnccdHandler::_fillQuadrant(const uint16_t* d, unsigned x, unsigned y)
{
  const int ppb = PixelsPerBin;

  bool lped    = (_options & PnccdCalib::option_no_pedestal())==0;
  bool lcommon = (_options & PnccdCalib::option_correct_common_mode()) &&
    (_cmth.size()>0);

  const unsigned o = Offset;
  const unsigned cx = (x==0) ? 0 : 4;
  const unsigned len = cols_segment/4;
  int32_t row[cols_segment];
  for(unsigned j=0; j<rows_segment; j++) {
    for(unsigned i=0; i<cols_segment; i++) {
      if (!lped)
        row[i] = *d++ + o;
      else
        row[i] = *d++ + o - int32_t(_ped[y+j][x+i]);
    }        
    if (lcommon)
      for(unsigned i0=0; i0<4; i0++) {
        int32_t cm, cmth = int32_t(_cmth[j+y][i0+cx])+o;
        int32_t* mrow = &row[i0*len];
        psalg::commonMode((const int32_t*)mrow,0,len,cmth,cmth,cm);
        cm -= o;
        for(unsigned i=0; i<len; i++)
          mrow[i] -= cm;
      }

    if (ppb==2) {
      unsigned iy = (j+y)>>1;
      if ((_entry->desc().options() & PnccdCalib::option_rotate())==0) {
        for(unsigned i=0, ix=x; i<rows_segment; i++, ix++)
          if ((i&1) || (j&1))
            _entry->addcontent(row[i],ix>>1,iy);
          else
            _entry->   content(row[i],ix>>1,iy);
      }
      else {
        for(unsigned i=0, ix=1023-x; i<rows_segment; i++, ix--)
          if ((i&1) || (j&1))
            _entry->addcontent(row[i],iy,ix>>1);
          else
            _entry->   content(row[i],iy,ix>>1);
      }
    }
    else {
      unsigned iy = y+j;
      if ((_entry->desc().options() & PnccdCalib::option_rotate())==0) {
        for(unsigned i=0, ix=x; i<rows_segment; i++, ix++)
          _entry->content(row[i],ix,iy);
      }
      else {
        for(unsigned i=0, ix=1023-x; i<rows_segment; i++, ix--)
          _entry->content(row[i],iy,ix);
      }
    }
  }
}

void PnccdHandler::_fillQuadrantR(const uint16_t* d, unsigned x, unsigned y)
{
  const int ppb = PixelsPerBin;

  bool lped    = (_options & PnccdCalib::option_no_pedestal())==0;
  bool lcommon = (_options & PnccdCalib::option_correct_common_mode()) &&
    (_cmth.size()>0);

  const unsigned o = Offset;
  const unsigned cx = (x>cols_segment) ? 7 : 3;
  const unsigned len = cols_segment/4;
  int32_t row[cols_segment];
  for(unsigned j=0; j<rows_segment; j++) {
    for(unsigned i=0; i<cols_segment; i++) {
      if (!lped)
        row[i] = *d++ + o;
      else
        row[i] = *d++ + o - int32_t(_ped[y-j][x-i]);
    }        
    if (lcommon)
      for(unsigned i0=0; i0<4; i0++) {
        int32_t cm, cmth = int32_t(_cmth[y-j][cx-i0])+o;
        int32_t* mrow = &row[i0*len];
        psalg::commonMode((const int32_t*)mrow,0,len,cmth,cmth,cm);
        cm -= o;
        for(unsigned i=0; i<len; i++)
          mrow[i] -= cm;
      }

    if (ppb==2) {
      unsigned iy = (y-j)>>1;
      if ((_entry->desc().options() & PnccdCalib::option_rotate())==0) {
        for(unsigned i=0, ix=x; i<rows_segment; i++, ix--)
          if ((i&1) || (j&1))
            _entry->addcontent(row[i],ix>>1,iy);
          else
            _entry->   content(row[i],ix>>1,iy);
      }
      else {
        for(unsigned i=0, ix=1023-x; i<rows_segment; i++, ix++)
          if ((i&1) || (j&1))
            _entry->addcontent(row[i],iy,ix>>1);
          else
            _entry->   content(row[i],iy,ix>>1);
      }
    }
    else {
      unsigned iy = y-j;
      if ((_entry->desc().options() & PnccdCalib::option_rotate())==0) {
        for(unsigned i=0, ix=x; i<rows_segment; i++, ix--)
          _entry->content(row[i],ix,iy);
      }
      else {
        for(unsigned i=0, ix=1023-x; i<rows_segment; i++, ix++)
          _entry->content(row[i],iy,ix);
      }
    }
  }
}

