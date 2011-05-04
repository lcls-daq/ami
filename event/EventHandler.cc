#include "EventHandler.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/Xtc.hh"

using namespace Ami;

EventHandler::EventHandler(const Pds::Src& info,
			   Pds::TypeId::Type   data_type,
			   Pds::TypeId::Type   config_type) :
  _info       (info),
  _data_type  (data_type)
{
  _config_type.push_back(config_type);
}

EventHandler::EventHandler(const Pds::Src& info,
			   Pds::TypeId::Type   data_type,
			   const std::list<Pds::TypeId::Type>& config_type) :
  _info       (info),
  _data_type  (data_type)
{
  _config_type.insert(_config_type.begin(),
		      config_type.begin(),
		      config_type.end());
}

EventHandler::~EventHandler()
{
}

void   EventHandler::_configure(Pds::TypeId type, 
				const void* payload, const Pds::ClockTime& t)
{
  _configure(payload,t);
}

void   EventHandler::_calibrate(Pds::TypeId type, 
				const void* payload, const Pds::ClockTime& t)
{
  _configure(payload,t);
}

void   EventHandler::_event(Pds::TypeId type, 
                            const void* payload, const Pds::ClockTime& t)
{
  _event(payload,t);
}

