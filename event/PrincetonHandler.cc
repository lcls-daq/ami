#include "PrincetonHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/psddl/princeton.ddl.h"
#include "pdsdata/xtc/Xtc.hh"
#include "pds/config/PrincetonDataType.hh"

#include <string.h>
#include <stdlib.h>

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

void PrincetonHandler::reset() { _entry = 0; }

void PrincetonHandler::_calibrate(Pds::TypeId type,const void* payload, const Pds::ClockTime& t) {}

void PrincetonHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
  if (_configtc) delete[] reinterpret_cast<char*>(_configtc);

  { const Xtc* tc = reinterpret_cast<const Xtc*>(payload)-1;
    _configtc = reinterpret_cast<Xtc*>(new char[tc->extent]);
    memcpy(_configtc, tc, tc->extent); }

  unsigned columns = width (_configtc);
  unsigned rows    = height(_configtc);
  unsigned pixels  = (columns > rows) ? columns : rows;
  unsigned ppb     = _full_resolution() ? 1 : (pixels-1)/640 + 1;
  columns = (columns+ppb-1)/ppb;
  rows    = (rows   +ppb-1)/ppb;
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, (unsigned)0, ChannelID::name(det),
     columns, rows, ppb, ppb);
  _entry  = new EntryImage(desc);

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

    ndarray<const uint16_t,2> a;
    switch(type.version()) {
    case 1: a = array<Pds::Princeton::FrameV1>(_configtc, payload); break;
    case 2: a = array<Pds::Princeton::FrameV2>(_configtc, payload); break;
    default: break;
    }

    const DescImage& desc = _entry->desc();
    unsigned ppbx = desc.ppxbin();
    unsigned ppby = desc.ppybin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    for(unsigned j=0; j<a.shape()[0]; j++)
      for(unsigned k=0; k<a.shape()[1]; k++)
        _entry->addcontent(a[j][k], k/ppbx, j/ppby);

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(0,EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);

    switch(type.version()) {
    case 1: break;
    case 2:
      _cache.cache(_iCacheIndexTemperature,
                   reinterpret_cast<const Pds::Princeton::FrameV2*>(payload)->temperature()); 
      break;
    default: break;
    }

  }
  else if (type.id() == Pds::TypeId::Id_PrincetonInfo)
  {
    const PrincetonInfoType& info1 = *reinterpret_cast<const PrincetonInfoType*>(payload);
    if (_iCacheIndexTemperature != -1)
      _cache.cache(_iCacheIndexTemperature, info1.temperature());
  }
}

void PrincetonHandler::_damaged() { _entry->invalid(); }
