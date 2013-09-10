#include "PnccdHandler.hh"

#include "ami/event/PnccdCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>

static const unsigned rows = 1024;
static const unsigned cols = 1024;
static const unsigned rows_segment = 512;
static const unsigned cols_segment = 512;

static const unsigned PixelVMask = 0x3fff;
static const unsigned Offset=1024;
static const unsigned MinCalib=25;

static int PixelsPerBin = 1;
static unsigned ArraySize=rows*cols/(PixelsPerBin*PixelsPerBin);

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
  _collect (false),
  _ncollect(0),
  _entry   (0),
  _tform   (true)
{
  PixelsPerBin = _full_resolution() ? 1 : 2;
  ArraySize    = rows*cols/(PixelsPerBin*PixelsPerBin);

  _correct = new EntryImage(DescImage(info, (unsigned)0, "PNCCD",
                                      cols / PixelsPerBin,
                                      rows / PixelsPerBin,
                                      PixelsPerBin, PixelsPerBin));
  _calib   = new PixelCalibration[ArraySize];
}
  
PnccdHandler::~PnccdHandler()
{
  delete[] _calib;
  delete   _correct;
}

unsigned PnccdHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* PnccdHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void PnccdHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void PnccdHandler::reset() { _entry = 0; }

void PnccdHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  _config = *reinterpret_cast<const Pds::PNCCD::ConfigV1*>(payload);

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, (unsigned)0, ChannelID::name(det),
		 cols / PixelsPerBin,
		 rows / PixelsPerBin,
		 PixelsPerBin, PixelsPerBin);
  _entry = new EntryImage(desc);

  char oname1[256];
  char oname2[256];
  sprintf(oname1,"rot.%08x.dat",info().phy());
  sprintf(oname2,"/reg/g/pcds/pds/pnccdcalib/%s",oname1);
  FILE* f = Calib::fopen_dual(oname1,oname2,"rotation");
  if (f) {
    _tform = false;
    fclose(f);
  }
  else
    _tform = true;

  PnccdCalib::load_pedestals(_correct,_tform);
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
      PnccdCalib::load_pedestals(_correct,_tform);
      _entry->desc().options( _entry->desc().options()&~PnccdCalib::option_reload_pedestal() );
    }
  }

  const Pds::PNCCD::FramesV1& f = *reinterpret_cast<const Pds::PNCCD::FramesV1*>(payload);
  _fillQuadrant (f.frame(_config,0).data(_config).data(), 0, 0);
  _fillQuadrant (f.frame(_config,1).data(_config).data(), cols_segment-1, rows-1);
  _fillQuadrant (f.frame(_config,2).data(_config).data(), cols-1, rows-1);
  _fillQuadrant (f.frame(_config,3).data(_config).data(), cols_segment, 0);

  double n = double(PixelsPerBin*PixelsPerBin);
  _entry->info(double(Offset)*n,EntryImage::Pedestal);
  _entry->info(1,EntryImage::Normalization);
  _entry->valid(t);

  _ncollect++;
}

void PnccdHandler::_damaged() { _entry->invalid(); }

void PnccdHandler::_begin_calib() 
{
  for(unsigned i=0; i<ArraySize; i++)
    _calib[i].reset();
  _collect =true;
  _ncollect=0;
}

void PnccdHandler::_end_calib()
{
  _collect=false;
  if (_ncollect >= MinCalib) {

    // compute, store to a file 
    for(unsigned iy=0,i=0; iy<rows/PixelsPerBin; iy++)
      for(unsigned ix=0; ix<cols/PixelsPerBin; ix++,i++)
        _correct->content(_calib[i].avg(_ncollect), ix, iy);
        
    //  Rename the old calibration file
    const int NameSize=128;
    char oname[NameSize], nname[NameSize];
    sprintf(oname,"/tmp/pnccd.%08x.dat",info().phy());
    sprintf(nname,"/tmp/pnccd.%08x.dat.",info().phy());
    time_t t = time(NULL);
    char* dstr = nname+strlen(nname);
    strftime(dstr, nname+NameSize-1-dstr, "%Y%m%D_%H%M%S", localtime(&t));
    ::rename(oname,nname);

    //  Store the new calibration file
    FILE* f = fopen(oname,"w");
    if (f) {
      int ppb = PixelsPerBin;
      for(unsigned iy=0; iy<rows; iy++)
        for(unsigned ix=0; ix<cols; ix++)
          fwrite(_correct->contents()+(ix/ppb)+(cols/ppb)*(iy/ppb), sizeof(uint32_t), 1, f);
      fclose(f);
    }
    else
      printf("Failed to write %s\n",oname);
  }
}

void PnccdHandler::_fillQuadrant(const uint16_t* d, unsigned x, unsigned y)
{
  //
  //  Common mode
  //
  //    determined for each column by summing pixel rows 2-33 (next to outer edge)
  //
  int32_t common[cols_segment];
  const int ppb = PixelsPerBin;

  bool lped    = (_options & PnccdCalib::option_no_pedestal())==0;
  bool lcommon = _options & PnccdCalib::option_correct_common_mode();
  if (lcommon) {
    if (ppb==2) {
      for(unsigned ix=0; ix<256; ) {
        for(unsigned i=0; i<8; i++,ix++) {
          const uint16_t* p  = 2*512 + (ix<<1) + d;
          const uint16_t* p1 = 3*512 + (ix<<1) + d;
          int32_t v = 0;
          for(unsigned iy=y; iy<16+y; iy++, p+=2*512, p1+=2*512) {
            v += 
              (p [0]&0x3fff) +
              (p [1]&0x3fff) +
              (p1[0]&0x3fff) +
              (p1[1]&0x3fff);
            if (lped)
              v -= _correct->content(ix+(x>>1),iy);
          }
          common[ix] = v/16;
        }
      }
    }
    else { // ppb = 1
      for(unsigned ix=0; ix<cols_segment; ) {
        for(unsigned i=0; i<8; i++,ix++) {
          const uint16_t* p  = 2*cols_segment + ix + d;
          int32_t v = 0;
          for(unsigned iy=y; iy<32+y; iy++, p+=cols_segment) {
            v += (*p & 0x3fff);
            if (lped)
              v -= _correct->content(ix+x,iy);
          }
          common[ix] = v/32;
        }
      }
    }
  }

  if (ppb==2) {
    unsigned iy = y>>1;
    for(unsigned j=0; j<rows_segment; j+=2,iy++,d+=cols_segment) {
      const uint16_t* d1 = d+cols_segment;
      unsigned ix = x>>1;
      for(unsigned k=0; k<cols_segment; k+=2,ix++) {
        unsigned v =
          (d [0]&0x3fff) + 
          (d [1]&0x3fff) + 
          (d1[0]&0x3fff) +
          (d1[1]&0x3fff);

        //       if (_collect)
        // 	_calib[iy*(cols>>1)+ix].add(v);

        v += Offset<<2;
        if (lped)
          v -= _correct->content(ix, iy);
        if (lcommon)
          v -= common[ix-(x>>1)];
        if (_tform)
          _entry->content(v, iy, 511-ix);
        else
          _entry->content(v, ix, iy);

        d  += 2;
        d1 += 2;
      }
    }
  }
  else {
    unsigned iy = y;
    for(unsigned j=0; j<rows_segment; j++,iy++) {
      unsigned ix = x;
      for(unsigned k=0; k<cols_segment; k++,ix++,d++) {
        unsigned v = *d & 0x3fff;

        //       if (_collect)
        // 	_calib[iy*(cols)+ix].add(v);

        v += Offset;
        if (lped)
          v -= _correct->content(ix, iy);
        if (lcommon)
          v -= common[ix-x];
        if (_tform)
          _entry->content(v, iy, 1023-ix);
        else
          _entry->content(v, ix, iy);
      }
    }
  }
}

void PnccdHandler::_fillQuadrantR(const uint16_t* d, unsigned x, unsigned y)
{
  //  Common mode
  int32_t common[cols_segment];
  const int ppb = PixelsPerBin;

  bool lped    = (_options & PnccdCalib::option_no_pedestal())==0;
  bool lcommon = _options & PnccdCalib::option_correct_common_mode();
  if (lcommon) {
    if (ppb==2) {
      for(unsigned ix=0; ix<256; ) {
        for(unsigned i=0; i<8; i++,ix++) {
          const uint16_t* p  = 2*512 + (ix<<1) + d;
          const uint16_t* p1 = 3*512 + (ix<<1) + d;
          int32_t v = 0;
          for(unsigned iy=(y>>1)-1; iy>(y>>1)-17; iy--, p+=2*512, p1+=2*512) {
            v += 
              (p [0]&0x3fff) +
              (p [1]&0x3fff) +
              (p1[0]&0x3fff) +
              (p1[1]&0x3fff);
            if (lped)
              v -= _correct->content((x>>1)-ix,iy);
          }
          common[ix] = v/16;
        }
      }
    }
    else {
      for(unsigned ix=0; ix<512; ) {
        for(unsigned i=0; i<8; i++,ix++) {
          const uint16_t* p  = d + 2*512 + ix;
          int32_t v = 0;
          for(unsigned iy=y-1; iy>y-33; iy--, p+=512) {
            v += (p[0]&0x3fff);
            if (lped)
              v -= _correct->content(x-ix,iy);
          }
          common[ix] = v/32;
        }
      }
    }
  }

  //  PixelsPerBin = 2
  if (ppb==2) {
    unsigned iy = y>>1;
    for(unsigned j=0; j<rows_segment; j+=2,iy--,d+=cols_segment) {
      const uint16_t* d1 = d+cols_segment;
      unsigned ix = x>>1;
      for(unsigned k=0; k<cols_segment; k+=2,ix--) {
        unsigned v = 
          (d [0]&0x3fff) + 
          (d [1]&0x3fff) + 
          (d1[0]&0x3fff) +
          (d1[1]&0x3fff);

        //       if (_collect)
        // 	_calib[iy*(cols>>1)+ix].add(v);

        v += Offset<<2;
        if (lped)
          v -= _correct->content(ix, iy);
        if (lcommon)
          v -= common[(x>>1)-ix];
        if (_tform)
          _entry->content(v, iy, 511-ix);
        else
          _entry->content(v, ix, iy);
        d  += 2;
        d1 += 2;
      }
    }
  }
  else {
    unsigned iy = y;
    for(unsigned j=0; j<rows_segment; j++,iy--) {
      unsigned ix = x;
      for(unsigned k=0; k<cols_segment; k++,ix--,d++) {
        unsigned v = (*d&0x3fff);

        //       if (_collect)
        // 	_calib[iy*cols+ix].add(v);

        v += Offset;
        if (lped)
          v -= _correct->content(ix, iy);
        if (lcommon)
          v -= common[x-ix];
        if (_tform)
          _entry->content(v, iy, 1023-ix);
        else
          _entry->content(v, ix, iy);
      }
    }
  }
}

