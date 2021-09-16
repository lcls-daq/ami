#include "FliHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static inline unsigned height(const Pds::Fli::ConfigV1& c)
{
  return c.height()/c.binY();
}

static unsigned width(const Pds::Fli::ConfigV1& c)
{
  return c.width()/c.binX();
}

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_FliFrame);
  return types;
}

FliHandler::FliHandler(const Pds::DetInfo& info, FeatureCache& cache) : 
  EventHandler(info, data_type_list(), Pds::TypeId::Id_FliConfig),
  _cache(cache),
  _iCacheIndexTemperature(-1),
  _entry(0)
{
}

//FliHandler::FliHandler(const Pds::DetInfo& info, const EntryImage* entry) : 
//  EventHandler(info, Pds::TypeId::Id_Frame, Pds::TypeId::Id_FliConfig),
//  _entry(entry ? new EntryImage(entry->desc()) : 0)
//{
//}

FliHandler::~FliHandler()
{
}

unsigned FliHandler::nentries() const { return _entry ? 1 : 0; }

const Entry* FliHandler::entry(unsigned i) const { return i==0 ? _entry : 0; }

void FliHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void FliHandler::reset() { _entry = 0; }

void FliHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{  
  if (type.version() == 1)
    _config = *reinterpret_cast<const Pds::Fli::ConfigV1*>(payload);
  else
    printf("FliHandler::_configure(): Unsupported Fli Version %d\n", type.version());    
  
  unsigned columns = width (_config);
  unsigned rows    = height(_config);
  unsigned ppb     = image_ppbin(columns, rows);
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescImage desc(det, (unsigned)0, ChannelID::name(det),
     columns, rows, ppb, ppb);
  _entry  = new EntryImage(desc);
    
  /*
   * Setup temperature variable
   */
  char sTemperatureVar[64];  
  sprintf(sTemperatureVar, "Fli-%d-T", det.devId());
  _iCacheIndexTemperature = _cache.add(sTemperatureVar);
}

void FliHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) {}

void FliHandler::_event    (Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  if (type.id() == Pds::TypeId::Id_FliFrame)
  {
    const Pds::Fli::FrameV1& f = *reinterpret_cast<const Pds::Fli::FrameV1*>(payload);
    if (!_entry) return;

    const DescImage& desc = _entry->desc();
    unsigned ppbx = desc.ppxbin();
    unsigned ppby = desc.ppybin();
    memset(_entry->contents(),0,desc.nbinsx()*desc.nbinsy()*sizeof(unsigned));
    const uint16_t* d = reinterpret_cast<const uint16_t*>(f.data(_config).data());
    for(unsigned j=0; j<height(_config); j++)
      for(unsigned k=0; k<width(_config); k++, d++)
        _entry->addcontent(*d, k/ppbx, j/ppby);

    //  _entry->info(f.offset()*ppbx*ppby,EntryImage::Pedestal);
    _entry->info(0,EntryImage::Pedestal);
    _entry->info(1,EntryImage::Normalization);
    _entry->valid(t);
    
    if (_iCacheIndexTemperature != -1)
      _cache.cache(_iCacheIndexTemperature, f.temperature());    
  }
}

void FliHandler::_damaged() { if (_entry) _entry->invalid(); }
