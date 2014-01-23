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
  int i = _cache.add(name);
  _indices .push_back(i);
  _features.push_back(std::string(name));
  return i;
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
  _indices .clear();
}

bool EventHandlerF::used() const 
{ 
  if (EventHandler::used()) return true;
  for(std::list<int>::const_iterator it=_indices.begin();
      it!=_indices.end(); it++)
    if (_cache.used(*it)) return true;
  return false;
}
