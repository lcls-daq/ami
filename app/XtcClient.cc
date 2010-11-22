#include "XtcClient.hh"

#include "ami/app/SummaryAnalysis.hh"

#include "ami/event/EventHandler.hh"
#include "ami/event/EvrHandler.hh"
#include "ami/event/FEEGasDetEnergyReader.hh"
#include "ami/event/EBeamReader.hh"
#include "ami/event/PhaseCavityReader.hh"
#include "ami/event/EpicsXtcReader.hh"
#include "ami/event/SharedIpimbReader.hh"
#include "ami/event/ControlXtcReader.hh"
#include "ami/event/IpimbHandler.hh"
#include "ami/event/EncoderHandler.hh"
#include "ami/event/Opal1kHandler.hh"
#include "ami/event/TM6740Handler.hh"
#include "ami/event/FccdHandler.hh"
#include "ami/event/PnccdHandler.hh"
#include "ami/event/CspadHandler.hh"
#include "ami/event/PrincetonHandler.hh"
#include "ami/event/AcqWaveformHandler.hh"
#include "ami/event/DiodeFexHandler.hh"
#include "ami/event/IpmFexHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/UserAnalysis.hh"
#include "ami/server/Factory.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

using namespace Ami;

XtcClient::XtcClient(FeatureCache& cache, 
		     Factory&      factory,
		     UserAnalysis* user,
		     bool          sync) :
  _cache  (cache),
  _factory(factory),
  _user   (user),
  _sync   (sync),
  _ready  (false)
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
  if (dg->seq.isEvent() && _ready) {
    _seq = &dg->seq;
    SummaryAnalysis::instance().clock(dg->seq.clock());
    if (_user) _user->clock(dg->seq.clock());

    iterate(&dg->xtc); 

    _entry->valid(_seq->clock());
    _factory.analyze();
  }
  else if (dg->seq.service() == Pds::TransitionId::Configure) {

    printf("XtcClient configure\n");

    _cache.clear();

    //  Cleanup previous entries
    _factory.discovery().reset();
    SummaryAnalysis::instance().reset();
    if (_user) _user->reset();
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++)
      (*it)->reset();

    _seq = &dg->seq;
    SummaryAnalysis::instance().clock(dg->seq.clock());
    if (_user) _user->clock(dg->seq.clock());
    
    iterate(&dg->xtc); 

    //  Create and register new entries
    Pds::DetInfo noInfo;
    _factory.discovery().add(_entry = new EntryScalar(noInfo,0,"XtcClient","timestamp"));
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      for(unsigned k=0; k<(*it)->nentries(); k++) {
	_factory.discovery().add   (const_cast<Entry*>((*it)->entry(k)));
      }
    }

    printf("XtcClient configure done\n");

    //  Advertise
    _factory.discover();
    if (_sync) _factory.wait_for_configure();

    _ready =  true;
  }
  else {
    if (dg->seq.service() == Pds::TransitionId::Unconfigure)
      _ready = false;

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
    if (_seq->service()==Pds::TransitionId::L1Accept) {
      SummaryAnalysis::instance().event    (xtc->src,
					    xtc->contains,
					    xtc->payload());
      if (_user) _user->event    (xtc->src,
				  xtc->contains,
				  xtc->payload());
    }
    else if (_seq->service()==Pds::TransitionId::Configure) {
      SummaryAnalysis::instance().configure(xtc->src,
					    xtc->contains,
					    xtc->payload());
      if (_user) _user->configure(xtc->src,
				  xtc->contains,
				  xtc->payload());
    }
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      EventHandler* h = *it;
      if (h->info().level() == xtc->src.level() &&
	  (h->info().phy  () == (uint32_t)-1 ||
	   h->info().phy  () == xtc->src.phy())) {
	if (_seq->isEvent() && xtc->contains.id()==h->data_type()) {
	  if (xtc->damage.value())
	    h->_damaged();
	  else
	    h->_event(xtc->payload(),_seq->clock());
	  return 1;
	}
	else {
	  const std::list<Pds::TypeId::Type>& types = h->config_types();
	  Pds::TypeId::Type type = xtc->contains.id();
	  for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
	      it != types.end(); it++) {
	    if (*it == type) {
	      if (_seq->service()==Pds::TransitionId::Configure)
		h->_configure(type, xtc->payload(), _seq->clock());
	      else if (_seq->service()==Pds::TransitionId::BeginCalibCycle)
		h->_calibrate(type, xtc->payload(), _seq->clock());
	      else
		continue;
	      return 1;
	    }
	  }
	}
      }
    }
    //  Wasn't handled
    if (_seq->service()==Pds::TransitionId::Configure) {
      const DetInfo& info = reinterpret_cast<const DetInfo&>(xtc->src);
      const BldInfo& bldInfo = reinterpret_cast<const BldInfo&>(xtc->src);
      EventHandler* h = 0;
      switch(xtc->contains.id()) {
      case Pds::TypeId::Id_AcqConfig:        h = new AcqWaveformHandler(info); break;
      case Pds::TypeId::Id_FrameFexConfig:
	switch(info.device()) {
	case Pds::DetInfo::Opal1000:         h = new Opal1kHandler     (info); break;
	case Pds::DetInfo::TM6740  :         h = new TM6740Handler     (info); break;
	default: break;
	} break;
      case Pds::TypeId::Id_FccdConfig  :     h = new FccdHandler       (info); break;
      case Pds::TypeId::Id_PrincetonConfig:  h = new PrincetonHandler  (info); break;
      case Pds::TypeId::Id_pnCCDconfig:      h = new PnccdHandler    (info,_cache); break;
      case Pds::TypeId::Id_CspadConfig:      h = new CspadHandler    (info,_cache); break;
      case Pds::TypeId::Id_ControlConfig:    h = new ControlXtcReader     (_cache); break;
      case Pds::TypeId::Id_Epics:            h = new EpicsXtcReader       (_cache); break;
      case Pds::TypeId::Id_FEEGasDetEnergy:  h = new FEEGasDetEnergyReader(_cache); break;
      case Pds::TypeId::Id_EBeam:            h = new EBeamReader          (_cache); break;
      case Pds::TypeId::Id_PhaseCavity:      h = new PhaseCavityReader    (_cache); break;
      case Pds::TypeId::Id_IpimbConfig:      h = new IpimbHandler    (info,_cache); break;
      case Pds::TypeId::Id_EncoderConfig:    h = new EncoderHandler  (info,_cache); break;
      case Pds::TypeId::Id_EvrConfig:        h = new EvrHandler      (info,_cache); break;
      case Pds::TypeId::Id_DiodeFexConfig:   h = new DiodeFexHandler (info,_cache); break;
      case Pds::TypeId::Id_IpmFexConfig:     h = new IpmFexHandler   (info,_cache); break;
      case Pds::TypeId::Id_SharedIpimb:      h = new SharedIpimbReader(bldInfo,_cache); break;
      default: break;
      }
      if (!h)
	printf("XtcClient::process cant handle type %d\n",xtc->contains.id());
      else {
	printf("XtcClient::process adding handler for info %s type %s\n",
	       Pds::DetInfo::name(info), Pds::TypeId::name(xtc->contains.id()));
	insert(h);
	h->_configure(xtc->contains.id(),xtc->payload(),_seq->clock());
      }
    }
  }
  return 1;
}
