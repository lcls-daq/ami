#include "OceanOpticsHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/EntryRef.hh"
#include "ami/data/ChannelID.hh"
#include "pdsdata/xtc/ClockTime.hh"

#include <stdio.h>

using namespace Ami;

OceanOpticsHandler::OceanOpticsHandler(const Pds::DetInfo& info) :
  EventHandler(info, Pds::TypeId::Id_OceanOpticsData, Pds::TypeId::Id_OceanOpticsConfig),
  _configBuffer(new char[sizeof(Pds::OceanOptics::ConfigV2)]),
  _iConfigVer(-1),
  _nentries(0)
{
}

//OceanOpticsHandler::OceanOpticsHandler(const Pds::DetInfo&   info,
//               const Pds::OceanOptics::ConfigV1& config) :
//  EventHandler(info, Pds::TypeId::Id_OceanOpticsData, Pds::TypeId::Id_OceanOpticsConfig),
//  _config(_default),
//  _nentries(0)
//{
//  Pds::ClockTime t;
//  _configure(Pds::TypeId(Pds::TypeId::Id_OceanOpticsConfig,1), &config, t);
//}

OceanOpticsHandler::~OceanOpticsHandler()
{
  if (_configBuffer) delete[] _configBuffer;
}

unsigned OceanOpticsHandler::nentries() const { return _nentries; }

const Entry* OceanOpticsHandler::entry(unsigned i) const
{
  if (i<_nentries)
    return _entry[i];
  else
    return NULL;
}

void OceanOpticsHandler::rename(const char* s)
{
  if (_entry[0]) _entry[0]->desc().name(s);
}

void OceanOpticsHandler::reset() {
  _nentries = 0;
}

void OceanOpticsHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void OceanOpticsHandler::_configure(Pds::TypeId typeId, const void* payload, const Pds::ClockTime& t)
{
  ndarray<const double,1> p;
  int                     iNumPixels = 0;
  if (typeId.version() == 1)
  {
    const Pds::OceanOptics::ConfigV1& c = *reinterpret_cast<const Pds::OceanOptics::ConfigV1*>(payload);
    memcpy(_configBuffer, &c, sizeof(Pds::OceanOptics::ConfigV1));
    _iConfigVer = 1;
    iNumPixels  = Pds::OceanOptics::DataV1::iNumPixels;
    p           = c.waveLenCalib();
  }
  else if (typeId.version() == 2)
  {
    const Pds::OceanOptics::ConfigV2& c = *reinterpret_cast<const Pds::OceanOptics::ConfigV2*>(payload);
    memcpy(_configBuffer, &c, sizeof(Pds::OceanOptics::ConfigV2));
    _iConfigVer = 2;
    switch (c.deviceType()) {
    case 0:
      iNumPixels = (int)Pds::OceanOptics::DataV1::iNumPixels;
      break;
    case 1:
      iNumPixels = (int)Pds::OceanOptics::DataV2::iNumPixels;
      break;
    case 2:
      iNumPixels = (int)Pds::OceanOptics::DataV3::iNumPixels;
      break;
    }
    p           = c.waveLenCalib();
  }
  else
    return;

  unsigned  channelNumber = 0;
  double    fMinX         = p[0];
  double    fMaxX         = p[0] + p[1]*double(iNumPixels - 1);

  const Pds::DetInfo& det = static_cast<const Pds::DetInfo&>(info());
  DescWaveform desc(det, channelNumber,ChannelID::name(det,channelNumber),
    "Wavelength [m]","Count", iNumPixels, fMinX, fMaxX);

  _entry[_nentries++] = new EntryWaveform(desc);
}

void OceanOpticsHandler::_event    (Pds::TypeId typeId, const void* payload, const Pds::ClockTime& t)
{
  if (_iConfigVer==-1)
    return;

#define PLOT_CORR(cfg,dat) {                                            \
    const Pds::OceanOptics::dat* d = reinterpret_cast<const Pds::OceanOptics::dat*>(payload); \
    const Pds::OceanOptics::cfg& c = *reinterpret_cast<const Pds::OceanOptics::cfg*>(_configBuffer); \
    if (c.nonlinCorrect()[0]==0) {                                      \
      ndarray<const uint16_t,1> a = d->data();                          \
      for (int j=0;j<Pds::OceanOptics::dat::iNumPixels;j++)             \
        entry->content( a[j],j);                                        \
    }                                                                   \
    else {                                                              \
      for (int j=0;j<Pds::OceanOptics::DataV3::iNumPixels;j++)          \
        entry->content( d->nonlinerCorrected(c, j),j);                  \
    }                                                                   \
  }

  EntryWaveform* entry  = _entry[0];
  if (typeId.version() == 1)
  {
    if (_iConfigVer==1)
      PLOT_CORR(ConfigV1,DataV1)
    else if (_iConfigVer==2)
      PLOT_CORR(ConfigV2,DataV1)
  }
  else if (typeId.version() == 2)
    PLOT_CORR(ConfigV2,DataV2)
  else if (typeId.version() == 3)
    PLOT_CORR(ConfigV2,DataV3)
  else
    return;

#undef PLOT_CORR

  entry->info(1,EntryWaveform::Normalization);
  entry->valid(t);
}

void OceanOpticsHandler::_damaged()
{
  for (unsigned i=0;i<_nentries;i++) {
    EntryWaveform* entry = _entry[i];
    if (entry) entry->invalid();
  }
}
