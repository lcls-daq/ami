#include "XtcClient.hh"

#include "ami/app/EventFilter.hh"
#include "ami/app/AnalysisFactory.hh"
#include "ami/app/NameService.hh"

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
#include "ami/event/L3THandler.hh"
#include "ami/event/IpimbHandler.hh"
#include "ami/event/EncoderHandler.hh"
#include "ami/event/UsdUsbHandler.hh"
#include "ami/event/Gsc16aiHandler.hh"
#include "ami/event/Opal1kHandler.hh"
#include "ami/event/OrcaHandler.hh"
#include "ami/event/QuartzHandler.hh"
//#include "ami/event/PhasicsHandler.hh"
#include "ami/event/TimepixHandler.hh"
#include "ami/event/RayonixHandler.hh"
#include "ami/event/TM6740Handler.hh"
#include "ami/event/FccdHandler.hh"
#include "ami/event/Fccd960Handler.hh"
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
#include "ami/event/PimaxHandler.hh"
#include "ami/event/ImpWaveformHandler.hh"
#include "ami/event/EpixWaveformHandler.hh"
#include "ami/event/EpixHandler.hh"
#include "ami/event/EpixHandlerT.hh"
#include "ami/event/TimeToolHandler.hh"
#include "ami/event/BldSpectrometerHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/UserModule.hh"
#include "ami/server/Factory.hh"

#include "pdsdata/compress/CompressedXtc.hh"

#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"
#include "pdsdata/xtc/XtcIterator.hh"
#include "pdsdata/xtc/Dgram.hh"

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <iomanip>

static void Destroy(Xtc*) {}

static Ami::XtcClient* _instance=0;

//#define DBUG

static double clockTimeDiff(const Pds::ClockTime& a,
                            const Pds::ClockTime& b)
{
  return double(a.seconds()-b.seconds()) +
    1.e-9*(double(a.nanoseconds())-double(b.nanoseconds()));
}

using namespace Ami;

XtcClient::XtcClient(std::vector<FeatureCache*>& cache,
                     Factory&      factory,
                     EventFilter&  filter,
         bool          sync) :
  _cache   (cache),
  _factory (factory),
  _filter  (filter),
  _sync    (sync),
  _ready   (false),
  _ptime_index    (-1),
  _ptime_acc_index(-1),
  _pltnc_index    (-1),
  _event_index    (-1),
  _evtim_index    (-1),
  _evrtm_index    (-1),
  _runno_index    (-1),
  _runtim         (0,0),
  _nevents        (0),
  _name_service   (0)
{
  _instance = this;
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
    _nevents++;

    if (_filter.accept(dg)) {

      cache.cache(_runno_index,_runno_value);
      cache.cache(_event_index,_seq->stamp().vector());
      cache.cache(_evtim_index,_seq->clock().asDouble());
      cache.cache(_evrtm_index,clockTimeDiff(_seq->clock(),_runtim));

      for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
        if ((*it)->data_type() == Pds::TypeId::NumberOf)
          (*it)->_event(dg->xtc.contains, dg->xtc.payload(), _seq->clock());
        else {
          //  Readout groups are problematic because they result in detectors that
          //  don't contribute to every event.  How to flag this? (equivalent to damage)
          (*it)->_damaged();
        }
      }

      iterate(&dg->xtc);

      _cache[PostAnalysis]->cache(cache);

      if (_filter.accept()) {
  accept = true;
  _entry.front()->valid(_seq->clock());
  _factory.analyze();
      }
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
    _runtim      = dg->seq.clock();
    _nevents     = 0;
  }
  else if (dg->seq.service() == Pds::TransitionId::Configure) {
#ifdef DBUG
    printf("XtcClient configure\n");
#endif
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

    if (_name_service) {
      delete _name_service;
      _name_service = 0;
    }

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
      for(std::list<UserModule*>::const_iterator it=_filter.modules().begin();
          it != _filter.modules().end(); it++,imod++) {
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
#ifdef DBUG
    printf("XC\n");
#endif
    _factory.discovery().sort();  // Need to set a consistent order across all server processes
    _factory.discovery().showentries();
    //    _factory.hidden   ().showentries();

    _ptime_index     = cache.add("ProcTime");
    _ptime_acc_index = cache.add("ProcTimeAcc");
    _pltnc_index     = cache.add("ProcLatency");
    _event_index     = cache.add("EventId");
    _evtim_index     = cache.add("EventTime");
    _evrtm_index     = cache.add("EventTimeR");
    _runno_index     = cache.add("RunNumber");

    _cache[PostAnalysis]->add(cache);
#ifdef DBUG
    printf("XtcClient configure done\n");
#endif
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

void XtcClient::_configure(Pds::Xtc* xtc, EventHandler* h)
{
  const char* infoName(0);
  switch(xtc->src.level()) {
  case Pds::Level::Source:
    infoName = Pds::DetInfo::name(static_cast<const DetInfo&>(xtc->src));
    break;
  case Pds::Level::Reporter:
    infoName = Pds::BldInfo::name(static_cast<const BldInfo&>(xtc->src));
    break;
  case Pds::Level::Event:
    break;
  default:
    printf("Default name lookup failed for src %08x.%08x!\n",
           xtc->src.phy(), xtc->src.log());
    break;
  }

  if (_name_service) {
    const char* name = _name_service->name(h->info());
    if (name)
      infoName = name;
  }

  const ClockTime* ct = &_seq->clock();
  time_t t = ct->seconds();
  char* time = ctime(&t);
  time[strlen(time) - 1] = '\0';

  h->_configure(xtc->contains, xtc->payload(), _seq->clock());
  h->rename(infoName);

  const int nentries = h->nentries();
  if (nentries == 0) {
    return;
  }
  for (int i = 0; i < nentries; i++) {
    const Entry* entry = h->entry(i);
    const DescEntry& desc = entry->desc();
    const_cast<DescEntry&>(desc).recorded(_recorded);  // flag recorded/unrecorded data
#ifdef DBUG
    const DescEntry* descPtr = &desc;
    const char* typeName = Pds::TypeId::name(xtc->contains.id());
    const char* name = descPtr->name();
    printf("%s XtcClient::_configure: %s (%s): entry[%d]=%s\n", time, infoName, typeName, i, name);
#endif
  }
}

int XtcClient::process(Pds::Xtc* xtc)
{
  if (xtc->extent < sizeof(Xtc) ||
      xtc->contains.id() >= TypeId::NumberOf)
    return 0;

  if (xtc->contains.id() == Pds::TypeId::Id_Xtc) {
    //  Recorded data has xtc version 0
    _recorded = _sync | (xtc->contains.version()==0);
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

    if (h->used()) {

      const std::list<Pds::TypeId::Type>& types = h->data_types();

      Pds::TypeId::Type type = xtc->contains.id();
      for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
    it != types.end(); it++) {
        if (*it == type) {
#ifdef DBUG
    printf("Src %08x.%08x  Type %08x handled by %p\n",
           xtc->src.log(),xtc->src.phy(),xtc->contains.value(),h);
#endif
                boost::shared_ptr<Xtc> pxtc = xtc->contains.compressed() ?
                  Pds::CompressedXtc::uncompress(*xtc) :
                  boost::shared_ptr<Xtc>(xtc,Destroy);

                h->_event(pxtc->contains,pxtc->payload(),_seq->clock(),pxtc->damage);
    return 1;
        }
        else
    continue;
      }
          }
#ifdef DBUG
    else {
      const std::list<Pds::TypeId::Type>& types = h->data_types();
      printf("Handler %p not used [",h);
      for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
    it != types.end(); it++)
        printf("%s,",TypeId::name(*it));
      printf("]\n");
    }
#endif
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
    if (_seq->service()==Pds::TransitionId::Configure &&
  xtc->damage.value()==0) {
      const DetInfo& info    = reinterpret_cast<const DetInfo&>(xtc->src);
      const BldInfo& bldInfo = reinterpret_cast<const BldInfo&>(xtc->src);
      FeatureCache& cache = *_cache[PreAnalysis];
      EventHandler* h = 0;

      switch(xtc->contains.id()) {
      case Pds::TypeId::Any:                 h = new EpixHandlerT      (info); break;
      case Pds::TypeId::Id_AcqConfig:        h = new AcqWaveformHandler(info); break;
      case Pds::TypeId::Id_AcqTdcConfig:     h = new AcqTdcHandler     (info); break;
      case Pds::TypeId::Id_ImpConfig:        h = new ImpWaveformHandler(info); break;
      case Pds::TypeId::Id_TM6740Config:     h = new TM6740Handler     (info); break;
      case Pds::TypeId::Id_Opal1kConfig:     h = new Opal1kHandler     (info); break;
      case Pds::TypeId::Id_OrcaConfig  :     h = new OrcaHandler       (info); break;
      case Pds::TypeId::Id_QuartzConfig:     h = new QuartzHandler     (info); break;
        //      case Pds::TypeId::Id_PhasicsConfig:    h = new PhasicsHandler    (info); break;
      case Pds::TypeId::Id_TimepixConfig:    h = new TimepixHandler    (info); break;
      case Pds::TypeId::Id_RayonixConfig:    h = new RayonixHandler    (info); break;
      case Pds::TypeId::Id_FccdConfig  :
        if (info.device()==DetInfo::Fccd960) h = new Fccd960Handler    (info, cache);
        else                                 h = new FccdHandler       (info);
        break;
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
      case Pds::TypeId::Id_PimaxConfig:      h = new PimaxHandler      (info,cache); break;
      case Pds::TypeId::Id_ControlConfig:    h = new ControlXtcReader  (cache); break;
      case Pds::TypeId::Id_L3TConfig:        h = new L3THandler        (cache); break;
      case Pds::TypeId::Id_Epics:            h = new EpicsXtcReader    (info,cache); break;
      case Pds::TypeId::Id_FEEGasDetEnergy:  h = new FEEGasDetEnergyReader(cache); break;
      case Pds::TypeId::Id_EBeam:            h = new EBeamReader          (cache); break;
      case Pds::TypeId::Id_PhaseCavity:      h = new PhaseCavityReader    (cache); break;
      case Pds::TypeId::Id_GMD:              h = new GMDReader            (cache); break;
      case Pds::TypeId::Id_Spectrometer:     h = new BldSpectrometerHandler(bldInfo); break;
      case Pds::TypeId::Id_IpimbConfig:      h = new IpimbHandler    (info,cache); break;
      case Pds::TypeId::Id_EncoderConfig:    h = new EncoderHandler  (info,cache); break;
      case Pds::TypeId::Id_UsdUsbConfig:     h = new UsdUsbHandler   (info,cache); break;
      case Pds::TypeId::Id_Gsc16aiConfig:    h = new Gsc16aiHandler  (info,cache); break;
      case Pds::TypeId::Id_EvsConfig:
      case Pds::TypeId::Id_EvrConfig:        h = new EvrHandler      (info,cache); break;
      case Pds::TypeId::Id_DiodeFexConfig:   h = new DiodeFexHandler (info,cache); break;
      case Pds::TypeId::Id_IpmFexConfig:     h = new IpmFexHandler   (info,cache); break;
      case Pds::TypeId::Id_GenericPgpConfig:
      case Pds::TypeId::Id_Epix10kConfig:
      case Pds::TypeId::Id_Epix100aConfig:
      case Pds::TypeId::Id_EpixConfig:       h = new EpixHandler     (info,cache); break;
      case Pds::TypeId::Id_EpixSamplerConfig:h = new EpixWaveformHandler(info,cache); break;
      case Pds::TypeId::Id_TimeToolConfig:   h = new TimeToolHandler  (info,cache); break;
      case Pds::TypeId::Id_SharedIpimb:      h = new SharedIpimbReader(bldInfo,cache); break;
      case Pds::TypeId::Id_SharedPim:        h = new SharedPimHandler     (bldInfo); break;
      case Pds::TypeId::Id_AliasConfig:
  /*  Apply new name service to discovered data */
        if (!_name_service)
          _name_service = new NameService;
        _name_service->append(*xtc);
        for(HList::iterator it = _handlers.begin(); it != _handlers.end(); it++) {
          EventHandler* h = *it;
          const char* name = _name_service->name(h->info());
          if (name) {
            h->rename(name);
          }
  }
        break;
      default: break;
      }
      if (!h) {
        if (xtc->contains.id()==Pds::TypeId::Id_TM6740Config ||
            xtc->contains.id()==Pds::TypeId::Id_EpicsConfig  ||
            xtc->contains.id()==Pds::TypeId::Id_AliasConfig  ||
            xtc->contains.id()==Pds::TypeId::Id_PartitionConfig)
          ;
        else
          printf("XtcClient::process cannot handle type %d\n",xtc->contains.id());
      }
      else {
  char buff[128];
  const char* infoName = buff;

  switch(xtc->src.level()) {
  case Level::Source  : infoName = Pds::DetInfo::name(info); break;
  case Level::Reporter: infoName = Pds::BldInfo::name(bldInfo); break;
  default:  sprintf(buff,"Proc %08x:%08x", info.log(), info.phy()); break;
  }

  if (_name_service) {
    const char* name = _name_service->name(h->info());
    if (name) {
      h->rename(name);
      strcpy(buff,name);
      infoName = buff;
    }
  }

        const char* typeName = Pds::TypeId::name(xtc->contains.id());
#ifdef DBUG
        printf("XtcClient::process: adding handler %p for info %s type %s\n", h, infoName, typeName);
#endif
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

void XtcClient::discover_wait() { reinterpret_cast<AnalysisFactory&>(_factory).discover_wait(); }

XtcClient* XtcClient::instance() { return _instance; }

std::list<const EventHandler*> XtcClient::handlers() const
{
  std::list<const EventHandler*> v;
  for(std::list<EventHandler*>::const_iterator it=_handlers.begin();
      it!=_handlers.end(); it++)
    v.push_back(*it);
  return v;
}

std::string XtcClient::dump() const
{
  std::ostringstream s;
  time_t t(_seq->clock().seconds());
  char buff[64];
  strftime(buff, 64, "%T", localtime(&t));
  s << "\tXtcClient : rdy " << (_ready ? 't':'f')
    << " : nevts " << _nevents
    << " : " << Pds::TransitionId::name(_seq->service()) 
    << " " << buff
    << "." << std::setfill('0') << std::setw(9) <<  _seq->clock().nanoseconds()
    << std::endl;
  return s.str();
}
