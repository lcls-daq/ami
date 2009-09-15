#include "EventHandler.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/Xtc.hh"

using namespace Ami;

EventHandler::EventHandler(const Pds::Src& info,
			   Pds::TypeId::Type   data_type,
			   Pds::TypeId::Type   config_type) :
  _info       (info),
  _data_type  (data_type),
  _config_type(config_type)
{
}

EventHandler::~EventHandler()
{
}
