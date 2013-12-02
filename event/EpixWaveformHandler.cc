#include "EpixWaveformHandler.hh"

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
  EventHandler(info, Pds::TypeId::Id_EpixSamplerElement, Pds::TypeId::Id_EpixSamplerConfig),
  _cache   (cache),
  _config_buffer(0),
  _nentries(0),
  _nref    (0)
{
  for(unsigned i=0; i<MaxEntries; i++)
    _features[i] = -1;
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
  for(unsigned i=0; i<MaxEntries; i++)
    _features[i] = -1;
}

void EpixWaveformHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void EpixWaveformHandler::_configure(Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::EpixSampler::ConfigV1& c = *reinterpret_cast<const Pds::EpixSampler::ConfigV1*>(payload);
#ifdef DBUG
  printf("EpixWaveformHandler::configure  nchannels %d  samples %d  clkHalfT %d\n",
	 c.numberOfChannels(), c.samplesPerChannel(), c.adcClkHalfT());
#endif
  unsigned channelMask = (1<<c.numberOfChannels())-1;
  unsigned channelNumber = 0;

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
      
  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  for(unsigned k=0; channelMask!=0; k++) {
    if (channelMask&1) {
      DescWaveform desc(det, channelNumber,
			ChannelID::name(det,channelNumber),
			"Time [s]","[ADU]",
			c.samplesPerChannel(), 0., 
			double(c.samplesPerChannel())*sampleInterval);
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

  char buff[64];
  for(unsigned i=0; i<c.numberOfChannels(); i++) {
    sprintf(buff,"%s:Temp_Ch[%d]",DetInfo::name(det),i);
    _features[i] = _cache.add(buff);
  }

  if (_config_buffer) delete[] _config_buffer;
  memcpy(_config_buffer = new char[c._sizeof()], &c, c._sizeof());
}

void EpixWaveformHandler::_event    (Pds::TypeId, const void* payload, const Pds::ClockTime& t)
{
  const Pds::EpixSampler::ElementV1& d = *reinterpret_cast<const Pds::EpixSampler::ElementV1*>(payload);
  const Pds::EpixSampler::ConfigV1& _config = *new(_config_buffer) Pds::EpixSampler::ConfigV1;

  const unsigned n = _nentries;
  const unsigned ns = _config.samplesPerChannel();

#ifdef DBUG
  printf("EpixWaveformHandler::event  nentries %d  samples %d\n", n, ns);
#endif
	
  ndarray<const uint16_t,2> wfs = d.frame(_config);
  for (unsigned i=0;i<n;i++) {
    EntryWaveform* entry = _entry[i];
    const uint16_t* data = &wfs[i][0];
    for (unsigned j=0;j<ns; j++) 
      entry->content(float(*data++),j);

    entry->info(1,EntryWaveform::Normalization);
    entry->valid(t);
  }

  for(unsigned i=0; i<_nref; i++)
    _ref[i]->valid(t);

  ndarray<const uint16_t,1> temps = d.temperatures(_config);
  for (unsigned i=0; i<n; i++) {
    _cache.cache(_features[i],getTemp(temps[i]));
  }
}

void EpixWaveformHandler::_damaged() 
{
  for(unsigned i=0; i<_nentries; i++)
    _entry[i]->invalid();

  for(unsigned i=0; i<_nref; i++)
    _ref[i]->invalid();
}

bool EpixWaveformHandler::used() const
{
  for(unsigned i=0; i<_nref; i++)
    if (_ref[i] && _ref[i]->desc().used()) 
      return true;
  for(unsigned i=0; i<MaxEntries; i++)
    if (_cache.used(_features[i]))
      return true;
  return false;
}

double getTemp(unsigned v)
{
  double dv = double(v)/double(0x2000)-1;
  return -0.4329*dv*1e3 + 243.4;
}
