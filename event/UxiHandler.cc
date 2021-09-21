#include "UxiHandler.hh"

#include "ami/event/UxiCalib.hh"
#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

static const unsigned offset=1<<16;

static inline unsigned num_frames(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                           \
  { const Pds::Uxi::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Uxi::ConfigV##v*>(tc->payload());  \
      return c.numberOfFrames(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;

}

static inline unsigned num_rows(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                           \
  { const Pds::Uxi::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Uxi::ConfigV##v*>(tc->payload());  \
      return c.height(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned num_columns(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                           \
  { const Pds::Uxi::ConfigV##v& c =                                   \
      *reinterpret_cast<const Pds::Uxi::ConfigV##v*>(tc->payload());  \
      return c.width(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned acq_count(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                         \
  { const Pds::Uxi::FrameV##v& d =                                  \
      *reinterpret_cast<const Pds::Uxi::FrameV##v*>(tc->payload()); \
      return d.acquisitionCount(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned timestamp(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                         \
  { const Pds::Uxi::FrameV##v& d =                                  \
      *reinterpret_cast<const Pds::Uxi::FrameV##v*>(tc->payload()); \
      return d.timestamp(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline double temperature(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                         \
  { const Pds::Uxi::FrameV##v& d =                                  \
      *reinterpret_cast<const Pds::Uxi::FrameV##v*>(tc->payload()); \
      return d.temperature(); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned height(const Xtc* tc)
{
  return num_rows(tc);
}

static inline unsigned width(const Xtc* tc)
{
  return num_columns(tc) * num_frames(tc);
}

template <class Frame>
static inline ndarray<const uint16_t,3> array(const Xtc* tc,
                                              const void* f)
{
  #define CASE_VSN(v) case v:                                           \
  { const Pds::Uxi::ConfigV##v& c =                                     \
      *reinterpret_cast<const Pds::Uxi::ConfigV##v*>(tc->payload());    \
    return reinterpret_cast<const Frame*>(f)->frames(c); }

  switch(tc->contains.version()) {
    CASE_VSN(1);
    //CASE_VSN(2);
    default: break;
  }
#undef CASE_VSN
  return ndarray<const uint16_t,3>();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_UxiFrame);
  return types;
}

UxiHandler::UxiHandler(const Pds::DetInfo& info, FeatureCache& cache) : 
  EventHandler(info, data_type_list(), Pds::TypeId::Id_UxiConfig),
  _configtc(0),
  _cache(cache),
  _entry        (0),
  _pedestal     (make_ndarray<unsigned>(0U,0,0)),
  _options      (0),
  _cacheIndexAcqCount(-1),
  _cacheIndexTs      (-1),
  _cacheIndexTemp    (-1)
{
}

UxiHandler::~UxiHandler()
{
  if (_configtc) delete[] reinterpret_cast<char*>(_configtc);
}

unsigned UxiHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* UxiHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void UxiHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);

  char buffer[64];
  snprintf(buffer,53,s);
  char* c = buffer+strlen(buffer);

  if (_cacheIndexAcqCount != -1) {
    sprintf(c,":ACQ_COUNT");
    _cache.rename(_cacheIndexAcqCount,buffer);
  }
  if (_cacheIndexTs != -1) {
    sprintf(c,":TS");
    _cache.rename(_cacheIndexTs,buffer);
  }
  if (_cacheIndexTemp != -1) {
    sprintf(c,":TEMP");
    _cache.rename(_cacheIndexTemp,buffer);
  }
}

void UxiHandler::reset() 
{
  _entry = 0; 
}

void UxiHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  { const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    if (_configtc) delete[] reinterpret_cast<char*>(_configtc);
    _configtc = reinterpret_cast<Xtc*>(new char[tc->extent]);
    memcpy(_configtc, tc, tc->extent); }

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  unsigned columns = width (_configtc);
  unsigned rows    = height(_configtc);
  unsigned ppb     = image_ppbin(columns, rows);

  DescImage desc(det, (unsigned)0, ChannelID::name(det),
                 columns, rows, ppb, ppb);
  _entry  = new EntryImage(desc);
  _entry->invalid();

  char buffer[64];
  strncpy(buffer,Pds::DetInfo::name(det),53);
  char* c = buffer+strlen(buffer);
  sprintf(c,":ACQ_COUNT");
  _cacheIndexAcqCount = _cache.add(buffer);
  sprintf(c,":TS");
  _cacheIndexTs = _cache.add(buffer);
  sprintf(c,":TEMP");
  _cacheIndexTemp = _cache.add(buffer);
    
  _load_pedestals(num_frames(_configtc), num_rows(_configtc), num_columns(_configtc));
}

void UxiHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

void UxiHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_UxiFrame)
  {
    if (!_entry) return;

    const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    const DescImage& desc = _entry->desc();
    unsigned o = desc.options();
    if (_options != o) {
      printf("UxiHandler options %x -> %x\n",_options,o);
      _options = desc.options();
    }

    if (_cacheIndexAcqCount != -1) _cache.cache(_cacheIndexAcqCount, acq_count(tc));
    if (_cacheIndexTs != -1) _cache.cache(_cacheIndexTs, timestamp(tc));
    if (_cacheIndexTemp != -1) _cache.cache(_cacheIndexTemp, temperature(tc));

    unsigned frames  = num_frames(_configtc);
    unsigned columns = num_columns(_configtc);
    unsigned rows    = num_rows(_configtc);
    double   norm    = 1.0;
    
    if (desc.options() & UxiCalib::option_reload_pedestal()) {
      _load_pedestals(frames, rows, columns);
      _entry->desc().options( desc.options()&~UxiCalib::option_reload_pedestal() );
    }

    int ppbin = _entry->desc().ppxbin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    ndarray<const uint16_t, 3> d;
    switch(type.version()) {
      case 1:
        d = array<Pds::Uxi::FrameV1>(_configtc, payload);
        break;
      default:
        return;
    }

    for(unsigned i=0; i<frames; i++) {
      for(unsigned j=0; j<rows; j++) {
        for(unsigned k=0; k<columns; k++) {
          unsigned column_bin = k + (columns * i);
          unsigned pixel_val = d(i,j,k);
          unsigned calib_val = 0;
          if (!(desc.options()&UxiCalib::option_no_pedestal())) {
            calib_val = (offset + pixel_val) - _pedestal(i,j,k);
          } else {
            calib_val = offset + pixel_val;
          }
          _entry->addcontent((uint32_t) calib_val, column_bin/ppbin, j/ppbin);
        }
      }
    }

    _entry->info(double(offset*ppbin*ppbin),EntryImage::Pedestal);
    _entry->info(norm,EntryImage::Normalization);
    _entry->valid(t);
  }
}

void UxiHandler::_damaged() { if (_entry) _entry->invalid(); }

void UxiHandler::_load_pedestals(unsigned frames, unsigned rows, unsigned columns)
{
  const DescImage& d = _entry->desc();
  _pedestal = UxiCalib::load_array(d.info(), frames, rows, columns, 0U, NULL, "ped", "pedestals");
}
