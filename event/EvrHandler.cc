#include "EvrHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/evr/ConfigV5.hh"
#include "pdsdata/evr/ConfigV6.hh"
#include "pdsdata/evr/ConfigV7.hh"
#include "pdsdata/evr/DataV3.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>
#include <math.h>

using namespace Ami;

struct slot_s { 
  unsigned code;
  unsigned tick;
};

typedef struct slot_s slot_t;

static const slot_t slots[] = { 
  {   9, 12900 },
  {  10, 12951 },
  {  11, 12961 },
  {  12, 12971 },
  {  13, 12981 },
  {  14, 12991 },
  {  15, 13001 },
  {  16, 13011 },
  {  40, 12954 },
  {  41, 12964 },
  {  42, 12974 },
  {  43, 12984 },
  {  44, 12994 },
  {  45, 13004 },
  {  46, 13014 },
  { 162, 11840 } 
};

static unsigned timeslot(unsigned code)
{
  unsigned n = sizeof(slots)/sizeof(slot_t);
  for(unsigned i=0; i<n; i++)
    if (slots[i].code==code)
      return slots[i].tick;
  if (code >= 140 && code <= 159)
    return 11850+(code-140);
  if (code >= 67 && code <= 98)
    return 11900+code;
  if (code >= 167 && code <= 198)
    return 11900+code;
  return 0;
}

static int _delay_offs(const Pds::EvrData::ConfigV5& cfg,
		       unsigned pulse_id)
{
  for(unsigned i=0; i<cfg.neventcodes(); i++)
    if (cfg.eventcode(i).maskTrigger() & (1<<pulse_id)) {
      return int(timeslot(cfg.eventcode(i).code())) 
	-    int(timeslot(140));
    }

  return 0;
}

static int _delay_offs(const Pds::EvrData::ConfigV6& cfg,
		       unsigned pulse_id)
{
  for(unsigned i=0; i<cfg.neventcodes(); i++)
    if (cfg.eventcode(i).maskTrigger() & (1<<pulse_id)) {
      return int(timeslot(cfg.eventcode(i).code())) 
	-    int(timeslot(140));
    }

  return 0;
}

static int _delay_offs(const Pds::EvrData::ConfigV7& cfg,
		       unsigned pulse_id)
{
  for(unsigned i=0; i<cfg.neventcodes(); i++)
    if (cfg.eventcode(i).maskTrigger() & (1<<pulse_id)) {
      return int(timeslot(cfg.eventcode(i).code())) 
	-    int(timeslot(140));
    }

  return 0;
}

EvrHandler::EvrHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandler(info,
	       Pds::TypeId::Id_EvrData,
	       Pds::TypeId::Id_EvrConfig),
  _cache(f)
{
}

EvrHandler::~EvrHandler()
{
}

void   EvrHandler::_calibrate(Pds::TypeId type, const void* payload, const Pds::ClockTime& t) 
{ _configure(type,payload,t); }

#define REGISTER_CODE(evtcode) {		\
    unsigned code = evtcode;			\
    sprintf(iptr,"Evt%d",code);			\
    int index = _cache.add(buffer);		\
    _cache.cache(index, 0);			\
    _index[code] = index;			\
  }

void   EvrHandler::_configure(Pds::TypeId type, const void* payload, const Pds::ClockTime& t)
{
  char buffer[64];
  sprintf(buffer,"DAQ:EVR%d:",static_cast<const Pds::DetInfo&>(info()).devId());
  char* iptr = buffer+strlen(buffer);

  memset(_index, -1, sizeof(_index));

  switch(type.version()) {
  case 5:
    { const Pds::EvrData::ConfigV5& c = *reinterpret_cast<const Pds::EvrData::ConfigV5*>(payload);
      for(unsigned i=0; i<c.npulses(); i++) {
        sprintf(iptr,"P%d:Delay",i);
        int index = _cache.add(buffer);
        double delay = floor((c.pulse(i).delay()+_delay_offs(c,c.pulse(i).pulseId()))*c.pulse(i).prescale())/119.e6;
        _cache.cache(index, delay);
      }

      sprintf(buffer,"DAQ:EVR:");
      iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<c.neventcodes(); i++) {
        REGISTER_CODE(c.eventcode(i).code());
      }
      break; }
  case 6:
    { const Pds::EvrData::ConfigV6& c = *reinterpret_cast<const Pds::EvrData::ConfigV6*>(payload);
      for(unsigned i=0; i<c.npulses(); i++) {
        sprintf(iptr,"P%d:Delay",i);
        int index = _cache.add(buffer);
        double delay = floor((c.pulse(i).delay()+_delay_offs(c,c.pulse(i).pulseId()))*c.pulse(i).prescale())/119.e6;
        _cache.cache(index, delay);
      }

      sprintf(buffer,"DAQ:EVR:");
      iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<c.neventcodes(); i++) {
        REGISTER_CODE(c.eventcode(i).code());
      }
      break; }
  case 7:
    { const Pds::EvrData::ConfigV7& c = *reinterpret_cast<const Pds::EvrData::ConfigV7*>(payload);
      for(unsigned i=0; i<c.npulses(); i++) {
        sprintf(iptr,"P%d:Delay",i);
        int index = _cache.add(buffer);
        double delay = floor((c.pulse(i).delay()+_delay_offs(c,c.pulse(i).pulseId()))*c.pulse(i).prescale())/119.e6;
        _cache.cache(index, delay);
      }

      sprintf(buffer,"DAQ:EVR:");
      iptr = buffer+strlen(buffer);
      for(unsigned i=0; i<c.neventcodes(); i++) {
        REGISTER_CODE(c.eventcode(i).code());
      }
      break; }
  default:
    printf("type.version()=%d is not supported\n", type.version());
    return;
  }

  REGISTER_CODE(140);
  REGISTER_CODE(162);
}

#undef REGISTER_CODE

void   EvrHandler::_event    (Pds::TypeId type, const void* payload, const Pds::ClockTime& t) 
{
  for(unsigned i=0; i<256; i++)
    if (_index[i]>=0)
      _cache.cache(_index[i],0);

  if (type.version()==3) {
    const Pds::EvrData::DataV3& d = *reinterpret_cast<const Pds::EvrData::DataV3*>(payload);

    for(unsigned i=0; i<d.numFifoEvents(); i++) {
      int index = _index[d.fifoEvent(i).EventCode];
      if (index >=0) _cache.cache(index, 1);
    }
  }
}

void   EvrHandler::_damaged  () 
{
  for(unsigned i=0; i<256; i++)
    if (_index[i]>=0)
      _cache.cache(_index[i],0);
}

//  No Entry data
unsigned     EvrHandler::nentries() const { return 0; }
const Entry* EvrHandler::entry   (unsigned) const { return 0; }
void         EvrHandler::reset   () {}
