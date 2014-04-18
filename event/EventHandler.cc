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

void   EventHandler::_event    (Pds::TypeId id,
                                const void* payload, const Pds::ClockTime& t,
                                Pds::Damage dmg)
{
  if (dmg.value()==0)
    _event(id,payload,t);
  else
    _damaged();
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
static unsigned _res_limit = 640;
static bool _post_diagnostics = false;

void   EventHandler::enable_full_resolution(bool v) { _full_res = v; }

bool   EventHandler::_full_resolution() const { return _full_res; }

void   EventHandler::limit_resolution(unsigned p) { _res_limit=p; }

unsigned EventHandler::resolution() { return _res_limit; }

int    EventHandler::image_ppbin(unsigned& xpixels, unsigned& ypixels, unsigned margin)
{
  int ppbin = 1;
  if (!_full_res) {
    unsigned pixels  = (xpixels > ypixels) ? xpixels : ypixels;
    unsigned res_limit = _res_limit - 2*margin;
    if (pixels>res_limit/2) {
      ppbin   = (pixels-1)/res_limit + 1;
      xpixels = (xpixels+ppbin-1)/ppbin;
      ypixels = (ypixels+ppbin-1)/ppbin;
    }
  }
  return ppbin;
}

void   EventHandler::post_diagnostics(bool v) { _post_diagnostics=v; }

bool   EventHandler::post_diagnostics() { return _post_diagnostics; }
bool   EventHandler::used() const
{
  for(unsigned i=0; i<nentries(); i++)
    if (entry(i) && entry(i)->desc().used()) return true;
  return false;
}

std::list<std::string> EventHandler::features() const
{
  return std::list<std::string>();
}
