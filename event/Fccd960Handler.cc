#include "ami/event/Fccd960Handler.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "ami/event/FrameCalib.hh"
#include "ami/event/Calib.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/EntryImage.hh"
#include "pdsdata/psddl/camera.ddl.h"

using namespace Ami;

static const unsigned offset=1<<16;
static const double  doffset=double(offset);

static const Pds::TypeId Config_Type = Pds::TypeId(Pds::TypeId::Id_FccdConfig,2);
static const Pds::TypeId Data_Type   = Pds::TypeId(Pds::TypeId::Id_Frame,1);
static const unsigned Rows=960;
static const unsigned Columns=960;

static std::list<Pds::TypeId::Type> config_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FrameFexConfig);
  types.push_back(Pds::TypeId::Id_PimImageConfig);
  types.push_back(Pds::TypeId::Id_FccdConfig);
  return types;
}

static int frameNoise(ndarray<const uint32_t,1> data,
		      unsigned off)
{
  unsigned lo = off-100;
  unsigned hi = off+100;
  return FrameCalib::median(data,lo,hi)-int(off);
}


Fccd960Handler::Fccd960Handler(const Pds::Src& info, FeatureCache& cache) :
  EventHandler  (info, Data_Type.id(), config_type_list()),
  _cache        (cache),
  _entry        (0),
  _pentry       (0),
  _options      (0),
  _pedestals    (make_ndarray<unsigned>(1,1))
{
}

Fccd960Handler::~Fccd960Handler()
{
  if (_pentry)
    delete _pentry;
}

unsigned Fccd960Handler::nentries() const { return _entry ? 1 : 0; }

const Entry* Fccd960Handler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void Fccd960Handler::rename(const char* s)
{
  if (_entry)
    _entry->desc().name(s);
}

void Fccd960Handler::reset() 
{
  _entry = 0; 
  if (_pentry) { 
    delete _pentry; 
    _pentry=0; 
  }
}

void Fccd960Handler::_configure(Pds::TypeId tid, const void* payload, const Pds::ClockTime& t)
{
  if (tid.value() == Config_Type.value()) {
    const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());

    unsigned columns=Columns, rows=Rows;

    { int ppb = 1;
      DescImage desc(det, (unsigned)0, ChannelID::name(det),
		     columns, rows, ppb, ppb);
      _pentry = new EntryImage(desc);
      _pentry->invalid();
    }

    int ppb = image_ppbin(columns,rows);
    DescImage desc(det, (unsigned)0, ChannelID::name(det),
                   columns, rows, ppb, ppb);
    _entry = new EntryImage(desc);
    _entry->invalid();

    _load_pedestals();
    _load_gains    ();
  }
}

void Fccd960Handler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

bool Fccd960Handler::used() const 
{ 
  if (_entry && _entry->desc().used()) return true;
  return false;
}

void Fccd960Handler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  if (_entry && _entry->desc().used()) {
    unsigned o = _entry->desc().options();
    if (_options != o) {
      printf("Fccd960Handler::event options %x -> %x\n", _options, o);
      _options = o;
    }

    if (_entry->desc().options() & FrameCalib::option_reload_pedestal()) {
      _load_pedestals();
      _entry->desc().options( _entry->desc().options()&~FrameCalib::option_reload_pedestal() );
    }

    const Pds::Camera::FrameV1& f = *reinterpret_cast<const Pds::Camera::FrameV1*>(payload);

    _entry->reset();
    const DescImage& d = _entry->desc();

    const ndarray<const unsigned,2>& pa =
      d.options()&FrameCalib::option_no_pedestal() ?
      _offset : _pedestals;

    ndarray<const uint16_t,2> a = f.data16();

    int ppbin = _entry->desc().ppxbin();

    for(unsigned j=0; j<a.shape()[0]; j++) {
      const uint16_t* d = & a[j][0];
      const unsigned* p = &pa[j][0];
      for(unsigned k=0; k<a.shape()[1]; k++) {
        unsigned v = (d[k]&0x1fff) + p[k];
        _entry->addcontent(v, k/ppbin, j/ppbin);
      }
    }

    //
    //  correct each line even(odd) pixels in blocks of 160 columns
    //
    if (d.options()&FrameCalib::option_correct_common_mode2()) {
      int off = offset*d.ppxbin()*d.ppybin();
      ndarray<uint32_t,2> e = _entry->content();
      const int str[] = {2};
      for(unsigned i=0; i<e.shape()[0]; i++) {
        for(unsigned j=0; j<e.shape()[1]/160; j++)
          for(unsigned k=0; k<2; k++) {
            ndarray<uint32_t,1> s = make_ndarray<uint32_t>(&e[i][j*160+k],80);
            s.strides(str);
            unsigned iLo=off-128,iHi=off+128;
            int fn = FrameCalib::median(s,iLo,iHi)-off;
            uint32_t* v = &s[0];
            for(unsigned x=0; x<160; x+=2)
              v[x] -= fn;
          }
      }
    }

    //
    //  correct each block (10 columns x 480 rows)
    //
    if (d.options()&FrameCalib::option_correct_common_mode()) {
      int off = offset*d.ppxbin()*d.ppybin();
      ndarray<uint32_t,2> e = _entry->content();
      for(unsigned i=0; i<Rows/480; i++)
        for(unsigned j=0; j<Columns/10; j++) {
          ndarray<uint32_t,2> s = make_ndarray<uint32_t>(&e[i*480][j*10],480,10);
          s.strides(e.strides());
          unsigned iLo=off-128,iHi=off+128;
          int fn = FrameCalib::median(s,iLo,iHi)-off;
          for(unsigned y=0; y<480; y++) {
            uint32_t* v = &s[y][0];
            for(unsigned x=0; x<10; x++)
              v[x] -= fn;
          }
        }          
    }

    //
    //  Correct for gain
    //
    const ndarray<const double,2>& gn    = 
      (d.options()&FrameCalib::option_correct_gain()) ? _gain : _no_gain;
    {
    }

    _entry->info(double(offset*d.ppxbin()*d.ppybin()),EntryImage::Pedestal);
    _entry->info(1.,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void Fccd960Handler::_damaged() { if (_entry) _entry->invalid(); }

void Fccd960Handler::_load_pedestals()
{
  _pedestals    = make_ndarray<unsigned>(Rows,Columns);
  _offset       = make_ndarray<unsigned>(Rows,Columns);
  for(unsigned* a = _offset.begin(); a!=_offset.end(); *a++ = offset) ;

  EntryImage* p = _pentry;
  if (FrameCalib::load_pedestals(p,offset,"ped")) {
    for(unsigned *a=_pedestals.begin(), *b=p->contents(); a!=_pedestals.end(); *a++=*b++) ;
  }
  else
    for(unsigned* a=_pedestals.begin(); a!=_pedestals.end(); *a++=offset) ;
}

void Fccd960Handler::_load_gains()
{
  _gain    = make_ndarray<double>(Rows,Columns);
  _no_gain = make_ndarray<double>(Rows,Columns);

  for(double* a=_no_gain.begin(); a!=_no_gain.end(); *a++=1.) ;

  EntryImage* p = _pentry;
  ndarray<double,3> ap = FrameCalib::load(p->desc(),"gain");
  if (ap.size())
    for(double *a=_gain.begin(), *b=ap.begin(); a!=_gain.end(); *a++=*b++) ;
  else
    for(double* a=_gain.begin(); a!=_gain.end(); *a++=1.) ;
}


