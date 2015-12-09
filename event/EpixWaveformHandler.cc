#include "EpixWaveformHandler.hh"
#include "ami/event/Calib.hh"
#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "ami/data/FeatureCache.hh"
#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/psddl/epixsampler.ddl.h"

#include <stdio.h>

//#define DBUG

using namespace Ami;

static double getTemp(unsigned v);

EpixWaveformHandler::EpixWaveformHandler(const Pds::Src& info,
					 FeatureCache&   cache) : 
  EventHandlerF(info, Pds::TypeId::Id_EpixSamplerElement, Pds::TypeId::Id_EpixSamplerConfig, cache),
  _config_buffer(0),
  _nentries(0),
  _nref    (0)
{
}

EpixWaveformHandler::~EpixWaveformHandler()
{
  if (_config_buffer) delete[] _config_buffer;
}

#if 1
unsigned EpixWaveformHandler::nentries() const { return _nref; }

const Entry* EpixWaveformHandler::entry(unsigned i) const { return _ref[i]; }
#else
unsigned EpixWaveformHandler::nentries() const { return _nref ? 1:0; }

const Entry* EpixWaveformHandler::entry(unsigned i) const { return _ref[0]; }
#endif

void EpixWaveformHandler::rename(const char* s)
{
  char buff[64];
  
  for(unsigned i=0; i<_nentries; i++) {
    sprintf(buff,"%s_%02d",s,i+1);
    _entry[i]->desc().name(buff);
  }
  for(unsigned i=0; i<_nref; i++) {
    sprintf(buff,"%s_%02d_%02d",s,i*EntriesPerRef+1,(i+1)*EntriesPerRef);
    _ref[i]->desc().name(buff);
  }
}

void EpixWaveformHandler::reset() {
  _nentries = 0;
  _nref     = 0;
}

void EpixWaveformHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void EpixWaveformHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::EpixSampler::ConfigV1& c = *reinterpret_cast<const Pds::EpixSampler::ConfigV1*>(payload);
#ifdef DBUG
  printf("EpixWaveformHandler::configure  nchannels %d  samples %d  clkHalfT %d\n",
	 c.numberOfChannels(), c.samplesPerChannel(), c.adcClkHalfT());
#endif

  if (_config_buffer) delete[] _config_buffer;
  memcpy(_config_buffer = new char[c._sizeof()], &c, c._sizeof());

  unsigned channelMask = (1<<c.numberOfChannels())-1;
  unsigned channelNumber = 0;
  { Pds::DetInfo::Device dev =static_cast<const Pds::DetInfo&>(info()).device();
    if (dev==Pds::DetInfo::Epix || dev==Pds::DetInfo::Epix10k)
      channelNumber = 1;
  }

  double sampleInterval = 20.e-9;
  //  double sampleInterval = c.sampleInterval_sec();
#if 0
  double sampleInterval;
  { // BCD format (kHz)
    double v=0;
    for (unsigned r=c.baseClockFrequency(); r!=0; r>>=4)
      v += 10*(r&0xf);
    sampleInterval = double(c.adcClkHalfT())*2.e-3/v;
  }
#endif

  Ami::Calib::load_array(_gain  ,info().phy(),"gain","channel gains");
  Ami::Calib::load_array(_filter,info().phy(),"fir","filter weights");

  ndarray<double,1> roi;
  Ami::Calib::load_array(roi    ,info().phy(),"roi","waveform roi");

  _first_sample =  0;
  _last_sample  =  c.samplesPerChannel()-1;
  if (roi.size()) {
    _first_sample = unsigned(roi[0]/sampleInterval);
    _last_sample  = unsigned(roi[1]/sampleInterval);
  }

  if (_first_sample > _last_sample) {
    unsigned s = _first_sample;
    _first_sample = _last_sample;
    _last_sample  = s;
  }

  { 
    unsigned nweights = _filter.shape()[0];
    if (nweights==0) nweights=1;
    if (_last_sample+nweights > c.samplesPerChannel())
      _last_sample = c.samplesPerChannel()-nweights;
  }

  unsigned nsamples = 1+_last_sample-_first_sample;

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  for(unsigned k=0; channelMask!=0; k++) {
    if (channelMask&1) {
      DescWaveform desc(det, channelNumber,
			ChannelID::name(det,channelNumber),
			"Time [s]","[ADU]",
			nsamples, double(_first_sample)*sampleInterval,
			double(_last_sample+1)*sampleInterval);
      _entry[_nentries++] = new EntryWaveform(desc);
      channelNumber++;
    }
    channelMask >>= 1;
  }

  channelMask = (1<<c.numberOfChannels())-1;
  unsigned mask = (1<<EntriesPerRef)-1;
  for(unsigned i=0; channelMask!=0; i++) {
    unsigned m = channelMask & mask;
    if (m) {
      Channel ch(m,Channel::BitMask);
      (_ref[_nref++] = new EntryRef(DescRef(det,ch,ChannelID::name(det,ch))))->set(&_entry[i*EntriesPerRef]);
    }
    channelMask &= ~m;
    mask <<= EntriesPerRef;
  }

  _feature = make_ndarray<int>(c.numberOfChannels());
  char buff[64];
  for(unsigned i=0; i<c.numberOfChannels(); i++) {
    sprintf(buff,"%s:WF:Temp_Ch[%d]",DetInfo::name(det),i);
    _feature[i] = _add_to_cache(buff);
  }
}

void EpixWaveformHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::EpixSampler::ElementV1& d = *reinterpret_cast<const Pds::EpixSampler::ElementV1*>(payload);
  const Pds::EpixSampler::ConfigV1& _config = *new(_config_buffer) Pds::EpixSampler::ConfigV1;

  const unsigned n = _nentries;
  const unsigned ns = _last_sample-_first_sample+1;

#ifdef DBUG
  printf("EpixWaveformHandler::event  nentries %d  samples %d\n", n, ns);
#endif
	
  ndarray<const uint16_t,2> wfs = d.frame(_config);
  for (unsigned i=0;i<n;i++) {
    EntryWaveform* entry = _entry[i];
    const uint16_t* data = &wfs[i][_first_sample];
    double g = i<_gain.shape()[0] ? _gain[i] : 1;
    if (_filter.shape()[0]) {
      for (unsigned j=0;j<ns; j++, data++) {
        double v=0;
        for (unsigned k=0; k<_filter.shape()[0]; k++)
          v += float(data[k])*_filter[k];
        entry->content(v*g,j);
      }
    }
    else {
      for (unsigned j=0;j<ns; j++) 
        entry->content(float(*data++)*g,j);
    }

    entry->info(1,EntryWaveform::Normalization);
    entry->valid(t);
  }

  for(unsigned i=0; i<_nref; i++)
    _ref[i]->valid(t);

  ndarray<const uint16_t,1> temps = d.temperatures(_config);
  for (unsigned i=0; i<n; i++) {
    _cache.cache(_feature[i],getTemp(temps[i]));
  }
}

void EpixWaveformHandler::_damaged() 
{
  for(unsigned i=0; i<_nentries; i++)
    if (_entry[i]) _entry[i]->invalid();

  for(unsigned i=0; i<_nref; i++)
    if (_ref[i]) _ref[i]->invalid();
}

double getTemp(unsigned v)
{
  double dv = double(v)/double(0x2000)-1;
  return -0.4329*dv*1e3 + 243.4;
}

