#include "AcqTdcHandler.hh"

#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

static const unsigned MaxHits = 1000;
static const double TDC_PERIOD = 50e-12;
using namespace Ami;

AcqTdcHandler::AcqTdcHandler(const Pds::DetInfo& info) : 
  EventHandler(info, Pds::TypeId::Id_AcqTdcData, Pds::TypeId::Id_AcqTdcConfig),
  _entry(NULL)
{
}

AcqTdcHandler::AcqTdcHandler(const Pds::DetInfo&     info, 
			     const Pds::Acqiris::TdcConfigV1& config) :
  EventHandler(info, Pds::TypeId::Id_AcqTdcData, Pds::TypeId::Id_AcqTdcConfig),
  _entry(NULL)
{
  Pds::ClockTime t;
  _configure(Pds::TypeId(Pds::TypeId::Id_AcqTdcConfig,1), &config, t);
}

AcqTdcHandler::~AcqTdcHandler()
{
}

unsigned AcqTdcHandler::nentries() const { return (_entry != NULL); }

const Entry* AcqTdcHandler::entry(unsigned i) const { return (i == 0 ? _entry : NULL); }

void AcqTdcHandler::rename(const char* s)
{
  if (_entry) _entry->desc().name(s);
}

void AcqTdcHandler::reset() { _entry = NULL; }

void AcqTdcHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void AcqTdcHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  _entry = new EntryRef(DescRef(det,0,ChannelID::name(det,0)));
}

void AcqTdcHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  _entry->set(payload);
  _entry->valid(t);
}

void AcqTdcHandler::_damaged() 
{
  _entry->invalid();
}
