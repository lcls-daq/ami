#include "XtcClient.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

#include "ami/event/EventHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/server/Factory.hh"

using namespace Ami;

XtcClient::XtcClient(FeatureCache& cache, 
		     Factory&  factory,
		     bool      sync) :
  _cache  (cache),
  _factory(factory),
  _sync   (sync)
{
}

XtcClient::~XtcClient()
{
}

void XtcClient::insert(EventHandler* h) { _handlers.push_back(h); }
void XtcClient::remove(EventHandler* h) { _handlers.remove(h); }

void XtcClient::processDgram(Pds::Dgram* dg) 
{
  //  if (dg->seq.isEvent() && dg->xtc.damage.value()==0) {
  if (dg->seq.isEvent()) {
    _seq = &dg->seq;
    iterate(&dg->xtc); 

    _entry->valid(_seq->clock());
    _factory.analyze();
  }
  else if (dg->seq.service() == Pds::TransitionId::Configure) {

    _cache.clear();

    //  Cleanup previous entries
    _factory.discovery().reset();
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++)
      (*it)->reset();

    _seq = &dg->seq;
    iterate(&dg->xtc); 

    //  Create and register new entries
    Pds::DetInfo noInfo;
    _factory.discovery().add(_entry = new EntryScalar(noInfo,0,"XtcClient","timestamp"));
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      for(unsigned k=0; k<(*it)->nentries(); k++) {
	_factory.discovery().add   (const_cast<Entry*>((*it)->entry(k)));
      }
    }

    //  Advertise
    _factory.discover();
    if (_sync) _factory.wait_for_configure();
  }
  else {
    _seq = &dg->seq;
    iterate(&dg->xtc); 
  }
}

int XtcClient::process(Pds::Xtc* xtc) 
{
  if (xtc->extent < sizeof(Xtc) ||
      xtc->contains.id() >= TypeId::NumberOf)
    return 0;

  if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
    iterate(xtc);
  }
  else {
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      EventHandler* h = *it;

      if (h->info().level() == xtc->src.level() &&
	  (h->info().phy  () == -1UL ||
	   h->info().phy  () == xtc->src.phy())) {
	if (_seq->isEvent() && xtc->contains.id()==h->data_type()) {
	  if (xtc->damage.value())
	    h->_damaged();
	  else
	    h->_event(xtc->payload(),_seq->clock());
	}
	else if (_seq->service()==Pds::TransitionId::Configure &&
		 xtc->contains.id()==h->config_type()) {
	  h->_configure(xtc->payload(),_seq->clock());
	}
	else if (_seq->service()==Pds::TransitionId::BeginCalibCycle &&
		 xtc->contains.id()==h->config_type()) {
	  h->_calibrate(xtc->payload(),_seq->clock());
	}
      }
    }
  }
  return 1;
}
