#include "FrameFexHandler.hh"

#include "ami/data/FeatureCache.hh"

#include "pdsdata/psddl/camera.ddl.h"
#include "pdsdata/xtc/DetInfo.hh"

#include <stdio.h>
#include <string.h>

using namespace Ami;

FrameFexHandler::FrameFexHandler(const Pds::DetInfo& info, FeatureCache& f) :
  EventHandlerF(info,
		Pds::TypeId::Id_TwoDGaussian,
		Pds::TypeId::Id_FrameFexConfig,
		f)
{
}

FrameFexHandler::~FrameFexHandler()
{
}

void   FrameFexHandler::rename(const char* s)
{
  if (_index<0) return;

  unsigned i=_index;
  char buffer[64];
  sprintf(buffer,"%s:INT",s); _rename_cache(i++,buffer);
  sprintf(buffer,"%s:X"  ,s); _rename_cache(i++,buffer);
  sprintf(buffer,"%s:Y"  ,s); _rename_cache(i++,buffer);
  sprintf(buffer,"%s:MAJ",s); _rename_cache(i++,buffer);
  sprintf(buffer,"%s:MIN",s); _rename_cache(i++,buffer);
  sprintf(buffer,"%s:PHI",s); _rename_cache(i++,buffer);
}

void   FrameFexHandler::_calibrate(const void* payload, const Pds::ClockTime& t) {}
void   FrameFexHandler::_configure(const void* payload, const Pds::ClockTime& t)
{
  const Pds::Camera::FrameFexConfigV1& d = *reinterpret_cast<const Pds::Camera::FrameFexConfigV1*>(payload);
  if (d.processing() == Pds::Camera::FrameFexConfigV1::NoProcessing) {
    _index = -1;
    return;
  }

  char buffer[64];

  strncpy(buffer,Pds::DetInfo::name(static_cast<const Pds::DetInfo&>(info())),60);
  char* iptr = buffer+strlen(buffer);
  
  sprintf(iptr,":INT"); _add_to_cache(buffer);
  sprintf(iptr,":X"  ); _add_to_cache(buffer);
  sprintf(iptr,":Y"  ); _add_to_cache(buffer);
  sprintf(iptr,":MAJ"); _add_to_cache(buffer);
  sprintf(iptr,":MIN"); _add_to_cache(buffer);
  sprintf(iptr,":PHI"); _add_to_cache(buffer);
}

void   FrameFexHandler::_event    (const void* payload, const Pds::ClockTime& t)
{
  if (_index<0) return;

  const Pds::Camera::TwoDGaussianV1& d = *reinterpret_cast<const Pds::Camera::TwoDGaussianV1*>(payload);

  unsigned i=_index;
  _cache.cache(i++, d.integral());
  _cache.cache(i++, d.xmean());
  _cache.cache(i++, d.ymean());
  _cache.cache(i++, d.major_axis_width());
  _cache.cache(i++, d.minor_axis_width());
  _cache.cache(i++, d.major_axis_tilt());
}

//  No Entry data
unsigned     FrameFexHandler::nentries() const { return 0; }
const Entry* FrameFexHandler::entry   (unsigned) const { return 0; }
