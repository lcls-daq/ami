#include "L3TModule.hh"

#include "ami/data/EntryFactory.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/service/BuildStamp.hh"

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
#include "ami/event/ImpWaveformHandler.hh"
#include "ami/event/EpixWaveformHandler.hh"
#include "ami/event/EpixHandler.hh"
#include "ami/event/EpixHandlerT.hh"
#include "ami/data/Analysis.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/FilterFactory.hh"

#include "pdsdata/compress/CompressedXtc.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/xtc/DetInfo.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include <sstream>
#include <fstream>
#include <sstream>
#include <string>

//#define DBUG

static void Destroy(void*) {}

using namespace Ami::L3T;

L3TModule::L3TModule() :
  _import       (0),
  _name_service (0),
  _discovery    ("Discovery"),
  _filter       (0)
{
  for(unsigned i=0; i<Ami::NumberOfSets; i++)
    _features.push_back(new FeatureCache);
}

L3TModule::~L3TModule() 
{
  if (_import) delete _import;
}

std::string L3TModule::name() const
{
  return std::string("Ami::L3T::L3TModule: ")
    +std::string(BuildStamp::tag())
    +std::string(": ")
    +std::string(BuildStamp::time());
}

std::string L3TModule::configuration() const
{
  return _import->stream();
}

/**
 **  Caches need to be cleared before configure
 **  Discovery Cds needs to be reset
 **  Handlers need to be reset
 **/
void L3TModule::pre_configure()
{
  if (_import)
    delete _import;

  for(std::list<EventHandler*>::iterator hit = _handlers.begin(); 
      hit != _handlers.end(); hit++)
    delete *hit;
  _handlers.clear();

  _import = new FilterImport;
  _import->parse_handlers(*this);

  _features[ PreAnalysis]->clear();
  _features[PostAnalysis]->clear();
  _discovery.reset();
  for(std::list<EventHandler*>::iterator hit = _handlers.begin(); 
      hit != _handlers.end(); hit++)
    (*hit)->reset();
  if (_name_service) {
    delete _name_service;
    _name_service = 0;
  }

  _config_complete=true;
}

/**
 **  Handler entries need to be registered after configure (Discovery)
 **  PreAnalysis cache added to PostAnalysis cache
 **
 **/
void L3TModule::post_configure()
{
  for(std::list<EventHandler*>::iterator it = _handlers.begin(); 
      it != _handlers.end(); it++) {
    std::list<int>::iterator sit = _signatures[(*it)->info().phy()].begin();
    for(unsigned k=0; k<(*it)->nentries(); k++,sit++) {                     
      const Entry* e = (*it)->entry(k);
      if (e) {
	unsigned signature = (*sit)>>16;
	unsigned options   = (*sit)&0xffff;
#ifdef DBUG
	printf("Assigning signature %d options %x to %s\n",
	       signature, options, e->desc().name());
#endif
        /// Assign the correct (fixed) signature
        _discovery.add   (const_cast<Entry*>(e),signature);
	/// Assign any special options that were in effect
	_discovery.entry(signature)->desc().options(options);
      }
    }
  }
  _features[PostAnalysis]->add(*_features[PreAnalysis]);

  for(std::list<Ami::Analysis*>::iterator it=_analyses.begin();
      it!=_analyses.end(); it++)
    delete (*it);
  _analyses.clear();

  for(std::map<int,Cds*>::iterator it=_cds.begin();
      it!=_cds.end(); it++)
    it->second->clear_used();

  _import->parse_analyses(*this);

  if (_filter) {
    delete _filter;
    _filter = 0;
  }

  _import->parse_filter  (*this);

#ifdef DBUG
  for(std::map<int,Cds*>::iterator it=_cds.begin();
      it!=_cds.end(); it++)
    it->second->showentries();
  const std::vector<std::string>& n = _features[PostAnalysis]->names();
  for(unsigned i=0; i<n.size(); i++)
    printf("\t%s\n",n[i].c_str());
#endif
}

void L3TModule::configure(const Pds::DetInfo&   src,
                          const Pds::TypeId&    type,
                          void*                 payload)
{
  _configure(src,type,payload);
}

void L3TModule::configure(const Pds::BldInfo&   src,
                          const Pds::TypeId&    type,
                          void*                 payload)
{
  _configure(src,type,payload);
}

void L3TModule::configure(const Pds::ProcInfo&  src,
                          const Pds::TypeId&    type,
                          void*                 payload)
{
  if (type.id()== Pds::TypeId::Id_AliasConfig) {
    const Xtc* xtc = reinterpret_cast<const Xtc*>(payload)-1;
    if (!_name_service)
      _name_service = new NameService;
    _name_service->append(*xtc);
    for(std::list<EventHandler*>::iterator it = _handlers.begin(); 
        it != _handlers.end(); it++) {
      EventHandler* h = *it;
      const char* name = _name_service->name(h->info());
      if (name) h->rename(name);
    }
  }
  else
    _configure(src,type,payload);
}

void L3TModule::_configure(const Pds::Src&       src,
                           const Pds::TypeId&    typeId,
                           void*                 payload)
{
  for(std::list<EventHandler*>::iterator it = _handlers.begin(); 
      it != _handlers.end(); it++) {
    EventHandler* h = *it;
    if (h->info().level() == src.level() &&
        (h->info().phy  () == (uint32_t)-1 ||
         h->info().phy  () == src.phy())) {
      const std::list<Pds::TypeId::Type>& types = h->config_types();
      Pds::TypeId::Type type = typeId.id();
      for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
          it != types.end(); it++) {
        if (*it == type) {
          h->_configure(typeId, payload, Pds::ClockTime(0,0));
          if (_name_service) {
            const char* name = _name_service->name(src);
            if (name)
              h->rename(name);
          }
          return;
        }
      }
    }
  }
}

void L3TModule::pre_event() 
{
  for(std::list<EventHandler*>::iterator it = _handlers.begin(); 
      it != _handlers.end(); it++)
    (*it)->_damaged();

  for(unsigned i=0; i<NumberOfSets; i++)
    _features[i]->start();
}

void L3TModule::event(const Pds::DetInfo& src,
                      const Pds::TypeId&  type,
                      void*               payload)
{
  _event(src,type,payload);
}

void L3TModule::event(const Pds::BldInfo& src,
                      const Pds::TypeId&  type,
                      void*               payload)
{
  _event(src,type,payload);
}

void L3TModule::event(const Pds::ProcInfo& src,
                      const Pds::TypeId&   type,
                      void*                payload)
{
  _event(src,type,payload);
}

void L3TModule::_event(const Pds::Src&     src,
                       const Pds::TypeId&  type,
                       void*               payload)
{
  for(std::list<EventHandler*>::iterator it = _handlers.begin(); 
      it != _handlers.end(); it++) {
    EventHandler* h = *it;
    if (h->info().level() == src.level() &&
        (h->info().phy  () == (uint32_t)-1 ||
         h->info().phy  () == src.phy())) {

#ifdef DBUG
      printf("Found handler [%p] for %08x.%08x used %c\n",
	     h, src.log(), src.phy(), h->used() ? 't':'f');
#endif

      if (!h->used()) return;

      const std::list<Pds::TypeId::Type>& types = h->data_types();

      Xtc* xtc = reinterpret_cast<Xtc*>(payload)-1;  // argh!!
      boost::shared_ptr<Xtc> pxtc = xtc->contains.compressed() ? 
        Pds::CompressedXtc::uncompress(*xtc) :
        boost::shared_ptr<Xtc>(xtc,Destroy);

      Pds::TypeId::Type type = pxtc->contains.id();
      for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
          it != types.end(); it++) {
        if (*it == type) {
#ifdef DBUG
	  printf("Handling type %s with damage %x\n",
		 Pds::TypeId::name(type), pxtc->damage.value());
#endif
          if (pxtc->damage.value())
            h->_damaged();
          else
            h->_event(pxtc->contains,pxtc->payload(),Pds::ClockTime(0,0));
          return;
        }
      }
    }
  }
}

bool L3TModule::complete()
{
  _features[PostAnalysis]->cache(*_features[PreAnalysis]);

  for(std::list<Analysis*>::iterator it=_analyses.begin();
      it!=_analyses.end(); it++)
    (*it)->analyze();

#ifdef DBUG
  printf("L3TModule::complete _filter %p  valid %c\n",
	 _filter, _filter->valid() ? 't':'f');
#endif
  return _filter && _filter->valid();
}

bool L3TModule::accept()
{
#if 0
  for(std::list<EventHandler*>::iterator it = _handlers.begin(); 
      it != _handlers.end(); it++) {
    if ((*it)->data_type() == Pds::TypeId::NumberOf)
      (*it)->_event(dg->xtc.contains, dg->xtc.payload(), _seq->clock());
  }
#endif

#ifdef DBUG
  printf("L3TModule::accept %c\n", _filter->accept() ? 't':'f');
#endif

  return _filter->accept();
}

void L3TModule::handler (const Pds::Src& src, 
                         const std::list<Pds::TypeId::Type>& types,
                         const std::list<int>& signatures)
{
  const DetInfo& info    = reinterpret_cast<const DetInfo&>(src);
  const BldInfo& bldInfo = reinterpret_cast<const BldInfo&>(src);
  FeatureCache& cache = *_features[PreAnalysis];
  EventHandler* h = 0;

  for(std::list<Pds::TypeId::Type>::const_iterator it=types.begin();
      it!=types.end(); it++) {
    switch(*it) {
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
    case Pds::TypeId::Id_EpixSamplerConfig:h = new EpixWaveformHandler(info,cache); break;
    case Pds::TypeId::Id_EpixConfig:       h = new EpixHandler     (info,cache); break;
    case Pds::TypeId::Id_SharedIpimb:      h = new SharedIpimbReader(bldInfo,cache); break;
    case Pds::TypeId::Id_SharedPim:        h = new SharedPimHandler     (bldInfo); break;
    default: break;
    }

    if (h) {
      _handlers.push_back(h);
      _signatures[h->info().phy()] = signatures;
      break;
    }
    else if (*it != Pds::TypeId::Id_FrameFexConfig) {
      printf("Ami::L3T::handler cannot handle type %s src %08x.%08x\n",Pds::TypeId::name(types.front()),src.log(),src.phy());
    }
  }
}

void L3TModule::analysis(unsigned id, 
                         ConfigureRequest::Source src,
                         unsigned input, 
                         unsigned output,
                         void*    op)
{
  if (_cds[id]==0) {
    std::stringstream cds_name;
    cds_name << "Analysis " << id;
    _cds[id] = new Cds(cds_name.str().c_str());
  }
  
  const Cds* pcds = 0;
  if  (src == ConfigureRequest::Discovery) 
    pcds = &_discovery;
  else
    pcds = _cds[id];

  const Entry* input_e = pcds->entry(input);
  if (!input_e) {
    printf("L3TModule::analysis failed to lookup input %s[%d]\n",
	   src==ConfigureRequest::Discovery ? "Discovery":"Analysis",input);
    _config_complete=false;
    return;
  }
	 
#ifdef DBUG
  printf("L3TModule::analysis creating from input %s/%d %p\n",
	 src==ConfigureRequest::Discovery ? "Discovery":"Analysis",input,input_e);
#endif

  const char* p = (const char*)op;
  Ami::Analysis* a = new Ami::Analysis(id, *input_e, output, *_cds[id],
                                       *_features[PostAnalysis], 
				       *_features[PostAnalysis],
				       p);
  a->input().desc().used(true);
  _analyses.push_back(a);
}

void L3TModule::filter  (const AbsFilter& filter)
{
  char* buffer = new char[4*1024];
  filter.serialize(buffer);

  const char* p = buffer;
  FilterFactory filters(*_features[PostAnalysis]);
  _filter = filters.deserialize(p);

  delete[] buffer;
}

extern "C" L3FilterModule* create() { return new L3TModule; }

