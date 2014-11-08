#include "ami/event/EventHandlerF.hh"

#include "ami/data/FeatureCache.hh"

using namespace Ami;

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     Pds::TypeId::Type   data_type,
			     Pds::TypeId::Type   config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache),
  _index(-1)
{}

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     const std::list<Pds::TypeId::Type>& data_type,
			     Pds::TypeId::Type   config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache),
  _index(-1)
{}

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     Pds::TypeId::Type   data_type,
			     const std::list<Pds::TypeId::Type>& config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache),
  _index(-1)
{}

EventHandlerF::EventHandlerF(const Pds::Src&     info,
			     const std::list<Pds::TypeId::Type>& data_type,
			     const std::list<Pds::TypeId::Type>& config_type,
			     FeatureCache&       cache) :
  EventHandler(info, data_type, config_type),
  _cache(cache),
  _index(-1)
{}
  
EventHandlerF::~EventHandlerF() {}

std::list<std::string> EventHandlerF::features() const { return _features; }

int EventHandlerF::_add_to_cache(const char* name)
{
  int i = _cache.add(name);
  _indices .push_back(i);
  _features.push_back(std::string(name));
  if (_index<0 || i<_index) _index=i;
  return i;
}

void EventHandlerF::_rename_cache(int index, const char* name)
{
  if (index<0) return;
  bool lFound=false;
  std::string fname(_cache.names()[index]);
  for(std::list<std::string>::iterator it=_features.begin();
      it!=_features.end(); it++)
    if ((*it)==fname) {
      (*it)=std::string(name);
      lFound=true;
      break;
    }
  if (lFound)
    _cache.rename(index,name);
}

void EventHandlerF::reset()
{
  _features.clear();
  _indices .clear();
  _index = -1;
}

bool EventHandlerF::used() const 
{ 
  if (EventHandler::used()) return true;
  for(std::vector<int>::const_iterator it=_indices.begin();
      it!=_indices.end(); it++)
    if (_cache.used(*it)) return true;
  return false;
}

void EventHandlerF::_damaged()
{
  for(std::vector<int>::const_iterator it=_indices.begin();
      it!=_indices.end(); it++)
    _cache.cache((*it),-1,true);
}
