#include "EventHandler.hh"

#include "ami/data/DescEntry.hh"
#include "ami/data/Entry.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Sequence.hh"
#include "pdsdata/xtc/Xtc.hh"

using namespace Ami;

EventHandler::EventHandler(const Pds::Src& info,
			   Pds::TypeId::Type   data_type,
			   Pds::TypeId::Type   config_type) :
  _info       (info)
{
  _data_type  .push_back(data_type  );
  _config_type.push_back(config_type);
}

EventHandler::EventHandler(const Pds::Src&     info,
                           const std::list<Pds::TypeId::Type>& data_type,
                           Pds::TypeId::Type   config_type) :
  _info       (info)
{
  _data_type  .insert(_data_type.begin(),
                      data_type.begin(),
                      data_type.end());
  _config_type.push_back(config_type  );
}

EventHandler::EventHandler(const Pds::Src& info,
			   Pds::TypeId::Type   data_type,
			   const std::list<Pds::TypeId::Type>& config_type) :
  _info       (info)
{
  _data_type  .push_back(data_type  );
  _config_type.insert(_config_type.begin(),
		      config_type.begin(),
		      config_type.end());
}

EventHandler::EventHandler(const Pds::Src&     info,
                           const std::list<Pds::TypeId::Type>& data_type,
                           const std::list<Pds::TypeId::Type>& config_type) :
  _info       (info)
{
  _data_type  .insert(_data_type.begin(),
                      data_type.begin(),
                      data_type.end());
  _config_type.insert(_config_type.begin(),
		      config_type.begin(),
		      config_type.end());
}

EventHandler::~EventHandler()
{
}

/*
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
*/

static bool _full_res = false;

void   EventHandler::enable_full_resolution(bool v) { _full_res = v; }

bool   EventHandler::_full_resolution() const { return _full_res; }

int    EventHandler::image_ppbin(int& xpixels, int& ypixels)
{
  int ppbin = 1;
  if (!_full_res) {
    const unsigned DISPLAY_SIZE=640;
    unsigned pixels  = (xpixels > ypixels) ? xpixels : ypixels;
    if (pixels>DISPLAY_SIZE/2) {
      ppbin   = (pixels-1)/DISPLAY_SIZE + 1;
      xpixels = (xpixels+ppbin-1)/ppbin;
      ypixels = (ypixels+ppbin-1)/ppbin;
    }
  }
  return ppbin;
}

bool   EventHandler::used() const
{
  for(unsigned i=0; i<nentries(); i++)
    if (entry(i)->desc().used()) return true;
  return false;
}

