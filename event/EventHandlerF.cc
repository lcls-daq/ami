#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

using namespace Ami;

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     Pds::TypeId::Type   data_type,
			     Pds::TypeId::Type   config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache)
{}

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     const std::list<Pds::TypeId::Type>& data_type,
			     Pds::TypeId::Type   config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache)
{}

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     Pds::TypeId::Type   data_type,
			     const std::list<Pds::TypeId::Type>& config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache)
{}

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     const std::list<Pds::TypeId::Type>& data_type,
			     const std::list<Pds::TypeId::Type>& config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache)
{}
  
EventHandlerF::~EventHandlerF() {}

std::list<std::string> EventHandlerF::features() const { return _features; }

int EventHandlerF::_add_to_cache(const char* name)
{
  _features.push_back(std::string(name));
  return _cache.add(name);
}

void EventHandlerF::_rename_cache(int index, const char* name)
{
  std::string fname(_cache.names()[index]);
  for(std::list<std::string>::iterator it=_features.begin();
      it!=_features.end(); it++)
    if ((*it)==fname) {
      (*it)=std::string(name);
      break;
    }
  _cache.rename(index,name);
}

void EventHandlerF::reset()
{
  _features.clear();
}

bool EventHandlerF::used() const { return true; }
