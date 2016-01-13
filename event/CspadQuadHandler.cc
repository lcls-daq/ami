#include "CspadQuadHandler.hh"
#include "CspadHandler.hh"

#include "ami/data/EntryImage.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/PeakFinder.hh"
#include "ami/data/PeakFinderFn.hh"

#include "pdsdata/psddl/cspad.ddl.h"
#include "pdsdata/xtc/Xtc.hh"

#include <sstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <limits.h>

typedef Pds::CsPad::ElementV2 CspadElement;

using namespace Ami;

static std::list<Pds::TypeId::Type> data_type_list()
{
  std::list<Pds::TypeId::Type> types;
  types.push_back(Pds::TypeId::Id_CspadElement);
  types.push_back(Pds::TypeId::Id_CspadCompressedElement);
  return types;
}

CspadQuadHandler::CspadQuadHandler(const Pds::DetInfo& info, FeatureCache& features) :
  EventHandlerF(info, data_type_list(), Pds::TypeId::Id_CspadConfig, features),
  _handlers    (4),
  _cfgPayload  (0)
{
  for(unsigned i=0; i<4; i++) {
    Pds::DetInfo q(info.processId(), info.detector(), 
                   info.detId(), info.device(), (1<<7)|(i<<4)|(info.devId()));
    _handlers[i] = new CspadHandler(q, features);
  }
}

CspadQuadHandler::~CspadQuadHandler()
{
  for(unsigned i=0; i<_handlers.size(); i++)
    delete _handlers[i];
  if (_cfgPayload)
    delete[] _cfgPayload;
}

unsigned CspadQuadHandler::nentries() const { 
  unsigned n=0;
  for(unsigned i=0; i<_handlers.size(); i++)
    n += _handlers[i]->nentries();
  return n;
}

const Entry* CspadQuadHandler::entry(unsigned i) const { 
  for(unsigned j=0; j<_handlers.size(); j++)
    if (i < _handlers[j]->nentries())
      return _handlers[j]->entry(i);
    else
      i -= _handlers[j]->nentries();
  return 0;
}

const Entry* CspadQuadHandler::hidden_entry(unsigned i) const { 
  return 0;
}


void CspadQuadHandler::rename(const char* s)
{
  for(unsigned i=0; i<_handlers.size(); i++) {
    std::ostringstream o;
    o << s << "_Q" << i;
    _handlers[i]->rename(o.str().c_str());
  }
}

void CspadQuadHandler::reset() { 
  for(unsigned i=0; i<_handlers.size(); i++)
    _handlers[i]->reset();
}

void CspadQuadHandler::_configure(Pds::TypeId type,const void* payload, const Pds::ClockTime& t)
{
#define CFG_CASE(VSN,QOFF,SOFF)                 \
  case VSN:                                     \
    { const Pds::CsPad::ConfigV##VSN& c = *reinterpret_cast<const Pds::CsPad::ConfigV##VSN*>(payload); \
      if (c.quadMask()&(1<<i)) {                                        \
        Pds::CsPad::ConfigV##VSN nc(c);                                 \
        reinterpret_cast<uint32_t*>(&nc)[QOFF]&=(1<<i);                 \
        reinterpret_cast<uint32_t*>(&nc)[SOFF]&=(0xff<<(8*i));          \
        _handlers[i]->_configure(type,&nc,t);                           \
        _cfgPayload = new char[c._sizeof()];                            \
        memcpy(_cfgPayload,&c,c._sizeof());                             \
      }                                                                 \
    } break

  _cfgType = type;
  if (_cfgPayload) delete[] _cfgPayload;

  for(unsigned i=0; i<_handlers.size(); i++) {
    switch(type.version()) {
    case 1: 
      { const Pds::CsPad::ConfigV1& c = *reinterpret_cast<const Pds::CsPad::ConfigV1*>(payload);
        if (c.quadMask()&(1<<i)) {
          Pds::CsPad::ConfigV1 nc(c);
          reinterpret_cast<uint32_t*>(&nc)[10]&=(1<<i);
          _handlers[i]->_configure(type,&nc,t);
        }
      } break;
    CFG_CASE(2,10,11);
    CFG_CASE(3,19,20);
    CFG_CASE(4,19,20);
    CFG_CASE(5,20,21);
    default:
      break;
    }
  }

#undef CFG_CASE
}

void CspadQuadHandler::_calibrate(Pds::TypeId id, const void* payload, const Pds::ClockTime& t) {
  for(unsigned i=0; i<_handlers.size(); i++)
    _handlers[i]->_calibrate(id, payload, t);
}

void CspadQuadHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  const Xtc* xtc = reinterpret_cast<const Xtc*>(payload)-1;

  _event(xtc->contains, xtc->payload(), xtc->sizeofPayload(), t);
}

void CspadQuadHandler::_event    (TypeId contains, const char* payload, size_t sizeofPayload, const Pds::ClockTime& t)
{
#define CHKTYPE(DV,CV) \
  if (contains.version()==DV && _cfgType.version()==CV) {               \
    const Pds::CsPad::ElementV##DV& e = *reinterpret_cast<const Pds::CsPad::ElementV##DV*>(payload); \
    quad   = e.quad();                                                  \
    sizeOf = e._sizeof(*reinterpret_cast<const Pds::CsPad::ConfigV##CV*>(_cfgPayload)); \
  }

  const char* const end = payload+sizeofPayload;
  while( payload < end ) {
    unsigned quad=0, sizeOf=0;
    CHKTYPE(1,1);
    CHKTYPE(1,2);
    CHKTYPE(1,3);
    CHKTYPE(1,4);
    CHKTYPE(1,5);
    CHKTYPE(2,2);
    CHKTYPE(2,3);
    CHKTYPE(2,4);
    CHKTYPE(2,5);
    if (!sizeOf) break;
    _handlers[quad]->_event(contains, payload, sizeOf, t);
    payload += sizeOf;
  }
}

void CspadQuadHandler::_damaged() 
{
  for(unsigned i=0; i<_handlers.size(); i++)
    _handlers[i]->_damaged();
}

