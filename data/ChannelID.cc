#include "ChannelID.hh"

#include "pdsdata/xtc/BldInfo.hh"

#include <string.h>
#include <stdio.h>

using namespace Pds;

static const int BSIZ=128;
static char _buffer[BSIZ];

#define NoChannel(title) { strcpy(_buffer,title,BSIZ); }
#define AcqChannel(title)                                       \
  if (info.device()==DetInfo::Acqiris) {                        \
    if (channel.is_mask())                                      \
      strcpy(_buffer,title);          \
    else                                                        \
      sprintf(_buffer,"%s_%d",title,channel+1);                 \
  }
#define OpalDetector(title)                             \
  if (info.device()==DetInfo::Opal1000 ||               \
      info.device()==DetInfo::Opal2000 ||               \
      info.device()==DetInfo::Opal4000) {               \
    if (info.devId()==0)                                \
      strcpy(_buffer,title);        \
    else                                                \
      sprintf(_buffer,"%s_%d",title,info.devId()+1);    \
  }
#define PnccdDetector                                   \
  if (info.device()==DetInfo::pnCCD)                    \
    sprintf(_buffer,"pnCCD_%d",info.devId()+1);

static void _default(char* b, const DetInfo& info, unsigned channel)
{
  if (info.device()==DetInfo::Acqiris || info.device()==DetInfo::Wave8) {
    Ami::Channel ch(channel);
    if (!ch.is_mask())
      sprintf(b,"%s_%s_%d",
              DetInfo::name(info.detector()),
              DetInfo::name(info.device  ()),
              channel+1);
    else
      sprintf(b,"%s_%s",
              DetInfo::name(info.detector()),
              DetInfo::name(info.device  ()));
  }
  else
    sprintf(b,"%s_%d_%s_%d",
      DetInfo::name(info.detector()),
            info.detId(),
      DetInfo::name(info.device  ()),
            info.devId());
}

const char* Ami::ChannelID::name(const Pds::DetInfo& info)
{ return Ami::ChannelID::name(info, Channel(0)); }

const char* Ami::ChannelID::name(const Pds::DetInfo& info, Channel channel)
{
  *_buffer = 0;
  if (info.level()==Pds::Level::Source) {
    switch(info.detector()) {
      //  AMO Detectors
    case DetInfo::AmoITof  : AcqChannel("ITOF"); break;
    case DetInfo::AmoIms   : AcqChannel("IMS"); break;
    case DetInfo::AmoETof  : AcqChannel("ETOF"); break;
    case DetInfo::AmoGasdet: AcqChannel("GASDET"); break;
    case DetInfo::AmoMbes  : AcqChannel("MBES"); break;
    case DetInfo::AmoBps   : OpalDetector("BPS"); break;
      //  CAMP Detectors
    case DetInfo::Camp     :
      AcqChannel("ACQ")
      else OpalDetector("VMI")
      else PnccdDetector
      else _default(_buffer,info,channel);
      break;
      //  Others
    case DetInfo::SxrBeamline:
      switch(info.device()) {
      case DetInfo::Opal1000:
        switch(info.devId()) {
        case 0 : strcpy(_buffer,"TSS_Opal"); break;
        case 1 : strcpy(_buffer,"EXS_Opal"); break;
        default: _default(_buffer,info,channel); break;
        }
        break;
      default: _default(_buffer,info,channel); break;
      }
      break;
    case DetInfo::SxrEndstation:
      switch(info.device()) {
      case DetInfo::Acqiris : {
        char buff[32];
        sprintf(buff,"ACQ%d",info.devId());
        AcqChannel(buff);
        break;
      }
      case DetInfo::Opal1000:
        sprintf(_buffer,"End_Opal_%d",info.devId()+1);
        break;
      case DetInfo::Princeton:
        sprintf(_buffer,"PI.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Fli:
        sprintf(_buffer,"Fli.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Andor:
        sprintf(_buffer,"Andor.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Zyla:
        sprintf(_buffer,"Zyla.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Jungfrau:
        sprintf(_buffer,"Jungfrau.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Pimax:
        sprintf(_buffer,"Pimax.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::ControlsCamera:
        sprintf(_buffer,"EpicsCam.%d.%d",info.detId(),info.devId());
        break;
      default: _default(_buffer,info,channel); break;
      }
      break;
    case DetInfo::XcsBeamline:
      if (info.device() == DetInfo::Ipimb && info.detId()>0 && info.detId()<3) {
        sprintf(_buffer,"XCS-%s-%02d",
                (info.detId()&1) ? "IPM":"DIO",
                info.devId());
        break;
      }
      else if (info.device() == DetInfo::TM6740) {
        sprintf(_buffer,"XCS-YAG-%02d",
                info.devId());
        break;
      } // else fall through
    default:
      switch(info.device()) {
      case DetInfo::TM6740:
        sprintf(_buffer,"%sCvd.%d.%d",
                Pds::DetInfo::name(info.detector()),
                info.detId(),info.devId());
        break;
      case DetInfo::Princeton:
        sprintf(_buffer,"PI.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Fli:
        sprintf(_buffer,"Fli.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Andor:
        sprintf(_buffer,"Andor.%d.%d",info.detId(),info.devId());
        break;
      case DetInfo::Zyla:
        sprintf(_buffer,"Zyla.%d.%d",info.detId(),info.devId());
        break;
      default:
        _default(_buffer,info,channel);
        break;
      }
      break;
    }
  }
  else if (info.level()==Pds::Level::Reporter) {
    const Pds::BldInfo& bld = reinterpret_cast<const Pds::BldInfo&>(info);
    sprintf(_buffer,"%s",Pds::BldInfo::name(bld));
  }
  return _buffer;
}

