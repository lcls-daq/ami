#include "PrincetonHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/psddl/princeton.ddl.h"
#include "pdsdata/xtc/Xtc.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace Pds;

static inline unsigned height(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Princeton::ConfigV##v& c =                               \
      *reinterpret_cast<const Pds::Princeton::ConfigV##v*>(tc->payload()); \
      return (c.height() + c.binY() - 1)/c.binY(); }

  switch(tc->contains.version()) {
    CASE_VSN(1)
    CASE_VSN(2)
    CASE_VSN(3)
    CASE_VSN(4)
    CASE_VSN(5)
      default: break;
  }
#undef CASE_VSN
  return 0;
}

static inline unsigned width(const Xtc* tc)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Princeton::ConfigV##v& c =                               \
      *reinterpret_cast<const Pds::Princeton::ConfigV##v*>(tc->payload()); \
      return (c.width() + c.binX() - 1)/c.binX(); }

  switch(tc->contains.version()) {
    CASE_VSN(1)
    CASE_VSN(2)
    CASE_VSN(3)
    CASE_VSN(4)
    CASE_VSN(5)
      default: break;
  }
#undef CASE_VSN
  return 0;
}

template <class Frame>
static inline ndarray<const uint16_t,2> array(const Xtc* tc,
                                              const void* f)
{
#define CASE_VSN(v) case v:                                             \
  { const Pds::Princeton::ConfigV##v& c =                               \
      *reinterpret_cast<const Pds::Princeton::ConfigV##v*>(tc->payload()); \
    return reinterpret_cast<const Frame*>(f)->data(c); }

  switch(tc->contains.version()) {
    CASE_VSN(1)
    CASE_VSN(2)
    CASE_VSN(3)
    CASE_VSN(4)
    CASE_VSN(5)
      default: break;
  }
#undef CASE_VSN
  return ndarray<const uint16_t,2>();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_PrincetonFrame);
  types.push_back(Pds::TypeId::Id_PrincetonInfo);
  return types;
}

PrincetonHandler::PrincetonHandler(const Pds::DetInfo& info, FeatureCache& cache) :
  EventHandler(info, data_type_list(), Pds::TypeId::Id_PrincetonConfig),
  _configtc(0),
  _cache(cache),
  _iCacheIndexTemperature(-1),
  _entry(0)
{
}

//PrincetonHandler::PrincetonHandler(const Pds::DetInfo& info, const EntryImage* entry) :
//  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_PrincetonConfig),
//  _entry(entry ? new EntryImage(entry->desc()) : 0)
//{
//}

PrincetonHandler::~PrincetonHandler()
{
  if (_configtc) delete[] reinterpret_cast<char*>(_configtc);
}

unsigned PrincetonHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* PrincetonHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void PrincetonHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void PrincetonHandler::reset() { _entry = 0; }

void PrincetonHandler::_calibrate(Pds::TypeId type,const void* payload, const Pds::ClockTime& t) {}

void PrincetonHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  if (_configtc) delete[] reinterpret_cast<char*>(_configtc);

  { const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    if (_configtc) delete[] reinterpret_cast<char*>(_configtc);
    _configtc = reinterpret_cast<Xtc*>(new char[tc->extent]);
    memcpy(_configtc, tc, tc->extent); }

  unsigned columns = width (_configtc);
  unsigned rows    = height(_configtc);
  int ppb = image_ppbin(columns,rows);

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, (unsigned)0, ChannelID::name(det),
     columns, rows, ppb, ppb);
  _entry  = new EntryImage(desc);
  _entry->invalid();

  /*
   * Setup temperature variable
   */
  char sTemperatureVar[64];
  sprintf(sTemperatureVar, "Princeton-%d-T", det.devId());
  _iCacheIndexTemperature = _cache.add(sTemperatureVar);
}

void PrincetonHandler::_event(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_PrincetonFrame)
  {
    if (!_entry) return;

    memset(_entry->contents(),0,_entry->desc().nbinsx()*_entry->desc().nbinsy()*sizeof(unsigned));

    ndarray<const uint16_t,2> a;
    switch(type.version()) {
    case 1: a = array<Pds::Princeton::FrameV1>(_configtc, payload); break;
    case 2: a = array<Pds::Princeton::FrameV2>(_configtc, payload); break;
    default: break;
    }
    _entry->content(a);

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(0,EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);

    switch(type.version()) {
    case 1: break;
    case 2:
      if (_iCacheIndexTemperature != -1)
      {
        float fTemperature = reinterpret_cast<const Pds::Princeton::FrameV2*>(payload)->temperature();
        if (fTemperature != 999) // temperature not defined
        _cache.cache(_iCacheIndexTemperature,fTemperature);
      }
      break;
    default: break;
    }

  }
  else if (type.id() == Pds::TypeId::Id_PrincetonInfo)
  {
    switch(type.version()) {
    case 1:
      { const Pds::Princeton::InfoV1& info1 = *reinterpret_cast<const Pds::Princeton::InfoV1*>(payload);
        if (_iCacheIndexTemperature != -1)
          _cache.cache(_iCacheIndexTemperature, info1.temperature());
      } break;
    default:
      break;
    }
  }
}

void PrincetonHandler::_damaged() { if (_entry) _entry->invalid(); }
