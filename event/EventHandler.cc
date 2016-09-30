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

int    EventHandler::display_ppbin(unsigned xpixels, unsigned ypixels, unsigned margin)
{
  unsigned pixels  = (xpixels > ypixels) ? xpixels : ypixels;
  unsigned res_limit = _res_limit - 2*margin;
  int ppbin = res_limit/pixels;
  if (ppbin==0) ppbin=1;
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

Pds::DetInfo EventHandler::info_mask() const 
{
  if (_info.level()==Pds::Level::Source) {
    Pds::DetInfo d(static_cast<const Pds::DetInfo&>(_info));
    if (d.devId()&(1<<7))
      return Pds::DetInfo(d.processId(),
                          d.detector(), d.detId(),
                          d.device()  , d.devId()&0x0f);
    return d;
  }
  return Pds::DetInfo((const char*)NULL);
}

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
#include "ami/event/UsdUsbFexHandler.hh"
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
#include "ami/event/CspadQuadHandler.hh"
#include "ami/event/CspadMiniHandler.hh"
#include "ami/event/PrincetonHandler.hh"
#include "ami/event/AcqWaveformHandler.hh"
#include "ami/event/AcqTdcHandler.hh"
#include "ami/event/DiodeFexHandler.hh"
#include "ami/event/IpmFexHandler.hh"
#include "ami/event/OceanOpticsHandler.hh"
#include "ami/event/FliHandler.hh"
#include "ami/event/AndorHandler.hh"
#include "ami/event/DualAndorHandler.hh"
#include "ami/event/PimaxHandler.hh"
#include "ami/event/ImpWaveformHandler.hh"
#include "ami/event/EpixWaveformHandler.hh"
#include "ami/event/EpixHandler.hh"
#include "ami/event/EpixHandlerT.hh"
#include "ami/event/TimeToolHandler.hh"
#include "ami/event/BldSpectrometerHandler.hh"
#include "ami/event/AnalogInputHandler.hh"
#include "ami/event/EOrbitsHandler.hh"
#include "ami/event/BeamMonitorHandler.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/event/Generic1DHandler.hh"

EventHandler* EventHandler::lookup(Pds::TypeId::Type id, const Pds::Src& src, FeatureCache& cache)
{
  EventHandler* h=0;
  const Pds::DetInfo& info    = static_cast<const Pds::DetInfo&>(src);
  const Pds::BldInfo& bldInfo = static_cast<const Pds::BldInfo&>(src);
  switch(id) {
  case Pds::TypeId::Any:                 h = new EpixHandlerT      (info); break;
  case Pds::TypeId::Id_AcqConfig:        h = new AcqWaveformHandler(info); break;
  case Pds::TypeId::Id_AcqTdcConfig:     h = new AcqTdcHandler     (info); break;
  case Pds::TypeId::Id_ImpConfig:        h = new ImpWaveformHandler(info); break;
  case Pds::TypeId::Id_Generic1DConfig:  h = new Generic1DHandler  (info); break;
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
    if (info.device()==DetInfo::Cspad) {
      if (info.detector()==DetInfo::MecTargetChamber)
        h = new CspadQuadHandler  (info,cache);
      else
        h = new CspadHandler      (info,cache);
    }
    else                                 h = new CspadMiniHandler  (info,cache);
    break;
  case Pds::TypeId::Id_Cspad2x2Config:   h = new CspadMiniHandler  (info,cache); break;
  case Pds::TypeId::Id_OceanOpticsConfig:h = new OceanOpticsHandler(info);     break;
  case Pds::TypeId::Id_FliConfig:        h = new FliHandler        (info,cache); break;
  case Pds::TypeId::Id_AndorConfig:      h = new AndorHandler      (info,cache); break;
  case Pds::TypeId::Id_Andor3dConfig:    h = new DualAndorHandler  (info,cache); break;
  case Pds::TypeId::Id_PimaxConfig:      h = new PimaxHandler      (info,cache); break;
  case Pds::TypeId::Id_ControlConfig:    h = new ControlXtcReader  (cache); break;
  case Pds::TypeId::Id_L3TConfig:        h = new L3THandler        (cache); break;
  case Pds::TypeId::Id_Epics:            
  case Pds::TypeId::Id_EpicsConfig:      h = new EpicsXtcReader    (info,cache); break;
  case Pds::TypeId::Id_FEEGasDetEnergy:  h = new FEEGasDetEnergyReader(cache); break;
  case Pds::TypeId::Id_EBeam:            h = new EBeamReader          (cache); break;
  case Pds::TypeId::Id_PhaseCavity:      h = new PhaseCavityReader    (cache); break;
  case Pds::TypeId::Id_GMD:              h = new GMDReader            (cache); break;
  case Pds::TypeId::Id_Spectrometer:     h = new BldSpectrometerHandler(bldInfo,cache); break;
  case Pds::TypeId::Id_IpimbConfig:      h = new IpimbHandler    (info,cache); break;
  case Pds::TypeId::Id_EncoderConfig:    h = new EncoderHandler  (info,cache); break;
  case Pds::TypeId::Id_UsdUsbConfig:     h = new UsdUsbHandler   (info,cache); break;
  case Pds::TypeId::Id_UsdUsbFexConfig:  h = new UsdUsbFexHandler(info,cache); break;
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
  case Pds::TypeId::Id_AnalogInput:      h = new AnalogInputHandler   (bldInfo,cache); break;
  case Pds::TypeId::Id_EOrbits:          h = new EOrbitsHandler       (bldInfo,cache); break;
  case Pds::TypeId::Id_BeamMonitorBldData: h = new BeamMonitorHandler (bldInfo,cache); break;
  default: break;
  }
  return h;
}
