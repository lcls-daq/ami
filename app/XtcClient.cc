#include "XtcClient.hh"

#include "ami/app/EventFilter.hh"

#include "ami/event/EventHandler.hh"
#include "ami/event/EvrHandler.hh"
#include "ami/event/FEEGasDetEnergyReader.hh"
#include "ami/event/EBeamReader.hh"
#include "ami/event/PhaseCavityReader.hh"
#include "ami/event/GMDReader.hh"
#include "ami/event/EpicsXtcReader.hh"
#include "ami/event/SharedIpimbReader.hh"
#include "ami/event/SharedPimHandler.hh"
#include "ami/event/ControlXtcReader.hh"
#include "ami/event/IpimbHandler.hh"
#include "ami/event/EncoderHandler.hh"
#include "ami/event/UsdUsbHandler.hh"
#include "ami/event/Gsc16aiHandler.hh"
#include "ami/event/Opal1kHandler.hh"
#include "ami/event/OrcaHandler.hh"
#include "ami/event/QuartzHandler.hh"
#include "ami/event/PhasicsHandler.hh"
#include "ami/event/TimepixHandler.hh"
#include "ami/event/TM6740Handler.hh"
#include "ami/event/FccdHandler.hh"
#include "ami/event/PnccdHandler.hh"
#include "ami/event/CspadHandler.hh"
#include "ami/event/CspadMiniHandler.hh"
#include "ami/event/PrincetonHandler.hh"
#include "ami/event/AcqWaveformHandler.hh"
#include "ami/event/AcqTdcHandler.hh"
#include "ami/event/DiodeFexHandler.hh"
#include "ami/event/IpmFexHandler.hh"
#include "ami/event/OceanOpticsHandler.hh"
#include "ami/event/FliHandler.hh"
#include "ami/event/AndorHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/UserModule.hh"
#include "ami/server/Factory.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

//#define DBUG

using namespace Ami;

XtcClient::XtcClient(std::vector<FeatureCache*>& cache, 
         Factory&      factory,
         UList&        user_ana,
                     EventFilter&  filter,
         bool          sync) :
  _cache   (cache),
  _factory (factory),
  _user_ana(user_ana),
  _filter  (filter),
  _sync    (sync),
  _ready   (false),
  _ptime_index    (-1),
  _ptime_acc_index(-1),
  _pltnc_index    (-1),
  _event_index    (-1),
  _runno_index    (-1)
{
}

XtcClient::~XtcClient()
{
}

void XtcClient::insert(EventHandler* h) { _handlers.push_back(h); }
void XtcClient::remove(EventHandler* h) { _handlers.remove(h); }

void XtcClient::processDgram(Pds::Dgram* dg) 
{
  bool accept=false;

  timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);

  FeatureCache& cache = *_cache[PreAnalysis];

#ifdef DBUG
  if (!dg->seq.isEvent())
    printf("%s %p\n",TransitionId::name(dg->seq.service()),dg);
#endif

  //  if (dg->seq.isEvent() && dg->xtc.damage.value()==0) {
  if (dg->seq.isEvent() && _ready) {
    _seq = &dg->seq;

    if (_filter.accept(dg)) {
      accept = true;

      cache.cache(_runno_index,_runno_value);
      cache.cache(_event_index,_seq->stamp().vector());
      for(UList::iterator it=_user_ana.begin(); it!=_user_ana.end(); it++)
        (*it)->clock(dg->seq.clock());
      
      iterate(&dg->xtc); 
      for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
        if ((*it)->data_type() == Pds::TypeId::NumberOf)
          (*it)->_event(dg->xtc.contains, dg->xtc.payload(), _seq->clock());
      }

      _cache[PostAnalysis]->cache(cache);

      _entry.front()->valid(_seq->clock());
      _factory.analyze();
    }
    //
    //  Time the processing
    //
    cache.start();
    timespec tq;
    clock_gettime(CLOCK_REALTIME, &tq);
    if (_ptime_index>=0) {
      double dt;
      dt = double(tq.tv_sec-tp.tv_sec) + 
        1.e-9*(double(tq.tv_nsec)-double(tp.tv_nsec));
      cache.cache(_ptime_index,dt);
      if (accept) {
        cache.cache(_ptime_acc_index,dt);
        _cache[PostAnalysis]->cache(_ptime_acc_index,dt);
      }

      dt = double(tq.tv_sec)-double(dg->seq.clock().seconds()) + 
        1.e-9*(double(tq.tv_nsec)-double(dg->seq.clock().nanoseconds()));
      cache.cache(_pltnc_index,dt);
      _cache[PostAnalysis]->cache(_pltnc_index,dt);
    }
  }
  else if (dg->seq.service() == Pds::TransitionId::BeginRun) {
    _runno_value = dg->env.value();
  }
  else if (dg->seq.service() == Pds::TransitionId::Configure) {

    printf("XtcClient configure\n");

    _cache[ PreAnalysis]->clear();
    _cache[PostAnalysis]->clear();

    //  Cleanup previous entries
    _factory.discovery().reset();
    _factory.hidden   ().reset();
    for(HList::iterator hit = _handlers.begin(); hit != _handlers.end(); hit++) {
      EventHandler* handler = *hit;
      handler->reset();
      if (handler->nentries() != 0) {
        printf("Warning: after reset, handler->nentries() = %d for handler %p\n", handler->nentries(), handler);
        const std::list<Pds::TypeId::Type>& types = handler->config_types();
        for (std::list<Pds::TypeId::Type>::const_iterator it=types.begin(); it != types.end(); it++) {
          const Pds::TypeId::Type& type = *it;
          printf("(handler %p type %s)\n", handler, Pds::TypeId::name(type));
        }
      }
    }

    _filter.reset();
    _filter.configure(dg);

    _seq = &dg->seq;
    for(UList::iterator it=_user_ana.begin(); it!=_user_ana.end(); it++)
      (*it)->clock(dg->seq.clock());
    
    // This adds entries back to the handlers via process()
    iterate(&dg->xtc); 

    //  Create and register new entries
    _entry.clear();
    { ProcInfo info(Pds::Level::Control,0,0);
      const DetInfo& dinfo = static_cast<const DetInfo&>((Src&)info);
      EntryScalar* e = new EntryScalar(dinfo,0,EntryScalar::input_entry(),"timestamp");
      _factory.discovery().add(e);
      _entry.push_back(e);
      int imod=0;
      for(UList::iterator it=_user_ana.begin(); it!=_user_ana.end(); it++,imod++) {
        info = ProcInfo(Pds::Level::Event,0,imod);
        e = new EntryScalar(dinfo,0,(*it)->name(),"module");
        _factory.discovery().add(e);
        _entry.push_back(e);
      }
    }

    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      for(unsigned k=0; k<(*it)->nentries(); k++) {                     
        const Entry* e = (*it)->entry(k);
        if (e) {
          unsigned signature = _factory.discovery().add   (const_cast<Entry*>(e));
          const Entry* o = (*it)->hidden_entry(k);
          if (o) 
            _factory.hidden().add   (const_cast<Entry*>(o),signature);
        }
      }
    }
    printf("XC\n");
    _factory.discovery().sort();  // Need to set a consistent order across all server processes
    _factory.discovery().showentries();
    //    _factory.hidden   ().showentries();

    _ptime_index     = cache.add("ProcTime");
    _ptime_acc_index = cache.add("ProcTimeAcc");
    _pltnc_index     = cache.add("ProcLatency");
    _event_index     = cache.add("EventId");
    _runno_index     = cache.add("RunNumber");

    _cache[PostAnalysis]->add(cache);

    printf("XtcClient configure done\n");

    //  Advertise
    _factory.discover(_sync);
    _ready =  true;
  }
  else {
    if (dg->seq.service() == Pds::TransitionId::Unconfigure)
      _ready = false;

    _seq = &dg->seq;
    iterate(&dg->xtc); 
  }
}

void XtcClient::_configure(Pds::Xtc* xtc, EventHandler* h) {
  const DetInfo& info = reinterpret_cast<const DetInfo&>(xtc->src);
  const char* infoName = Pds::DetInfo::name(info);
  const char* typeName = Pds::TypeId::name(xtc->contains.id());

  const ClockTime* ct = &_seq->clock();
  time_t t = ct->seconds();
  char* time = ctime(&t);
  time[strlen(time) - 1] = '\0';

  h->_configure(xtc->contains, xtc->payload(), _seq->clock());

  const int nentries = h->nentries();
  if (nentries == 0) {
    //printf("%s XtcClient::_configure: %s (%s): no entries\n", time, infoName, typeName);
    return;
  }
  for (int i = 0; i < nentries; i++) {
    const Entry* entry = h->entry(i);
    const DescEntry& desc = entry->desc();
    const DescEntry* descPtr = &desc;
    const char* name = descPtr->name();
    printf("%s XtcClient::_configure: %s (%s): entry[%d]=%s\n", time, infoName, typeName, i, name);
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
#ifdef DBUG
    if (_seq->service()==Pds::TransitionId::Configure) {
      printf("Type %s  Src %08x.%08x\n", 
	     TypeId::name(xtc->contains.id()),
	     xtc->src.phy(),xtc->src.log());
    }
#endif
    for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
      EventHandler* h = *it;
      if (h->info().level() == xtc->src.level() &&
          (h->info().phy  () == (uint32_t)-1 ||
           h->info().phy  () == xtc->src.phy())) {
        if (_seq->isEvent()) {
          const std::list<Pds::TypeId::Type>& types = h->data_types();
          Pds::TypeId::Type type = xtc->contains.id();
          for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
              it != types.end(); it++) {
            if (*it == type) {
              if (xtc->damage.value())
                h->_damaged();
              else
                h->_event(xtc->contains,xtc->payload(),_seq->clock());
            }
            else
              continue;
            return 1;
          }
        }
        else {
          const std::list<Pds::TypeId::Type>& types = h->config_types();
          Pds::TypeId::Type type = xtc->contains.id();
          for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
              it != types.end(); it++) {
            if (*it == type) {
              if (_seq->service()==Pds::TransitionId::Configure)
                _configure(xtc, h);
              else if (_seq->service()==Pds::TransitionId::BeginCalibCycle)
                h->_calibrate(xtc->contains, xtc->payload(), _seq->clock());
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
      const DetInfo& info    = reinterpret_cast<const DetInfo&>(xtc->src);
      const BldInfo& bldInfo = reinterpret_cast<const BldInfo&>(xtc->src);
      FeatureCache& cache = *_cache[PreAnalysis];
      EventHandler* h = 0;
      
      switch(xtc->contains.id()) {
      case Pds::TypeId::Id_AcqConfig:        h = new AcqWaveformHandler(info); break;
      case Pds::TypeId::Id_AcqTdcConfig:     h = new AcqTdcHandler     (info); break;
      case Pds::TypeId::Id_TM6740Config:     h = new TM6740Handler     (info); break;
      case Pds::TypeId::Id_Opal1kConfig:     h = new Opal1kHandler     (info); break;
      case Pds::TypeId::Id_OrcaConfig  :     h = new OrcaHandler       (info); break;
      case Pds::TypeId::Id_QuartzConfig:     h = new QuartzHandler     (info); break;
      case Pds::TypeId::Id_PhasicsConfig:    h = new PhasicsHandler    (info); break;
      case Pds::TypeId::Id_TimepixConfig:    h = new TimepixHandler    (info); break;
      case Pds::TypeId::Id_FccdConfig  :     h = new FccdHandler       (info); break;
      case Pds::TypeId::Id_PrincetonConfig:  h = new PrincetonHandler  (info, cache); break;
      case Pds::TypeId::Id_pnCCDconfig:      h = new PnccdHandler      (info,cache); break;
      case Pds::TypeId::Id_CspadConfig:      
        if (info.device()==DetInfo::Cspad)   h = new CspadHandler      (info,cache);
        else                                 h = new CspadMiniHandler  (info,cache);
        break;
      case Pds::TypeId::Id_Cspad2x2Config:   h = new CspadMiniHandler  (info,cache); break;
      case Pds::TypeId::Id_OceanOpticsConfig:h = new OceanOpticsHandler(info);     break;
      case Pds::TypeId::Id_FliConfig:        h = new FliHandler        (info,cache); break;
      case Pds::TypeId::Id_AndorConfig:      h = new AndorHandler      (info,cache); break;
      case Pds::TypeId::Id_ControlConfig:    h = new ControlXtcReader  (cache); break;
      case Pds::TypeId::Id_Epics:            h = new EpicsXtcReader    (info,cache); break;
      case Pds::TypeId::Id_FEEGasDetEnergy:  h = new FEEGasDetEnergyReader(cache); break;
      case Pds::TypeId::Id_EBeam:            h = new EBeamReader          (cache); break;
      case Pds::TypeId::Id_PhaseCavity:      h = new PhaseCavityReader    (cache); break;
      case Pds::TypeId::Id_GMD:              h = new GMDReader            (cache); break;
      case Pds::TypeId::Id_IpimbConfig:      h = new IpimbHandler    (info,cache); break;
      case Pds::TypeId::Id_EncoderConfig:    h = new EncoderHandler  (info,cache); break;
      case Pds::TypeId::Id_UsdUsbConfig:     h = new UsdUsbHandler   (info,cache); break;
      case Pds::TypeId::Id_Gsc16aiConfig:    h = new Gsc16aiHandler  (info,cache); break;
      case Pds::TypeId::Id_EvrConfig:        h = new EvrHandler      (info,cache); break;
      case Pds::TypeId::Id_DiodeFexConfig:   h = new DiodeFexHandler (info,cache); break;
      case Pds::TypeId::Id_IpmFexConfig:     h = new IpmFexHandler   (info,cache); break;
      case Pds::TypeId::Id_SharedIpimb:      h = new SharedIpimbReader(bldInfo,cache); break;
      case Pds::TypeId::Id_SharedPim:        h = new SharedPimHandler     (bldInfo); break;
      default: break;
      }
      if (!h) {
        if (xtc->contains.id()==Pds::TypeId::Id_TM6740Config ||
            xtc->contains.id()==Pds::TypeId::Id_EpicsConfig)
          ;
        else
          printf("XtcClient::process cannot handle type %d\n",xtc->contains.id());
      }
      else {
        const char* infoName = Pds::DetInfo::name(info);
        const char* typeName = Pds::TypeId::name(xtc->contains.id());
        printf("XtcClient::process: adding handler %p for info %s type %s\n", h, infoName, typeName);

#if 1
        // Sanity check -- a newly created handler should have no entries.
        int nentries = h->nentries();
        if (nentries > 0) {
          h->reset();
          nentries = h->nentries();
          if (nentries > 0) {
            fprintf(stderr, "XtcClient::process: nentries=%d for new handler %p info %s type %s!\n", nentries, h, infoName, typeName);
            *((char *) nentries) = 0; // force segfault for debugging
          }
        }
#endif

        insert(h);
        _configure(xtc, h);
      }
    }
  }
  return 1;
}
