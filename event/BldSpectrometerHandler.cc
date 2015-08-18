#include "BldSpectrometerHandler.hh"

#include "ami/data/EntryWaveform.hh"
#include "ami/data/FeatureCache.hh"

#include "pdsdata/xtc/ClockTime.hh"
#include "pdsdata/psddl/bld.ddl.h"

#include <stdio.h>
#include <string>
#include <sstream>

using std::ostringstream;
using std::string;

using namespace Ami;

static const unsigned MIN_PEAKS = 5;

BldSpectrometerHandler::BldSpectrometerHandler(const Pds::BldInfo& info, 
                                               FeatureCache& cache) : 
  EventHandlerF(info, 
                Pds::TypeId::Id_Spectrometer, 
                Pds::TypeId::Id_Spectrometer, 
                cache),
  _nentries(0)
{
}

BldSpectrometerHandler::~BldSpectrometerHandler()
{
}

unsigned BldSpectrometerHandler::nentries() const { return _nentries; }

const Entry* BldSpectrometerHandler::entry(unsigned i) const 
{
  if (i<_nentries)
    return _entry[i];
  else
    return 0;
}

void BldSpectrometerHandler::rename(const char* s)
{
  char buff[64];

  if (_nentries) {
    sprintf(buff,"%s_H",s);
    _entry[0]->desc().name(buff);
    if (_nentries>1) {
      sprintf(buff,"%s_V",s);
      _entry[1]->desc().name(buff);
    }
  }

  if (_index>=0) {
    string sname = string(s)+":";
    int index(_index);
    _rename_cache(index++,(sname+"comRaw")  .c_str());
    _rename_cache(index++,(sname+"com")     .c_str());
    _rename_cache(index++,(sname+"integral").c_str());
    for(unsigned i=0; i<_npeaks; i++) {
      string pname;
      { ostringstream o; o << sname << "peak" << i << ":"; pname=o.str(); }
      _rename_cache(index++,(pname+"pos" ).c_str());
      _rename_cache(index++,(pname+"ampl").c_str());
      _rename_cache(index++,(pname+"fwhm").c_str());
    }
  }
}

void BldSpectrometerHandler::reset() {
  EventHandlerF::reset();
  _nentries=0;
}

void BldSpectrometerHandler::_calibrate(Pds::TypeId, const void* payload, const Pds::ClockTime& t) {}

void BldSpectrometerHandler::_configure(Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  _nentries = 0;
  _npeaks   = MIN_PEAKS;
  _index    = -1;
  const char* name = BldInfo::name(static_cast<const BldInfo&>(info()));
  const DetInfo& dInfo = static_cast<const DetInfo&>(info());
  char buff[64];
  switch(id.version()) {
  case 0:
    { sprintf(buff,"%s_H",name);
      DescWaveform desc(dInfo, 0, buff,
                        "Pixel","Sum", 1024, 0., 1024.);
      _entry[_nentries++] = new EntryWaveform(desc); }
    { sprintf(buff,"%s_V",name);
      DescWaveform desc(dInfo, 1, buff,
                        "Pixel","Sum", 256, 0., 256.);
      _entry[_nentries++] = new EntryWaveform(desc); }
    break;
  case 1:
    { const Pds::Bld::BldDataSpectrometerV1& c = 
        *reinterpret_cast<const Pds::Bld::BldDataSpectrometerV1*>(payload);
      sprintf(buff,"%s_H",name);
      DescWaveform desc(dInfo, 0, buff,
                        "Pixel","Sum", c.width(), 0., float(c.width()));
      _entry[_nentries++] = new EntryWaveform(desc); 
      string sname = string(name) + ":";
//       if (c.nPeaks()>_npeaks)
// 	_npeaks = c.nPeaks();
      _add_to_cache((sname+"comRaw")  .c_str());
      _add_to_cache((sname+"com")     .c_str());
      _add_to_cache((sname+"integral").c_str());
      for(unsigned i=0; i<_npeaks; i++) {
        string pname;
        { ostringstream o; o << sname << "peak" << i << ":"; pname=o.str(); }
        _add_to_cache((pname+"pos" ).c_str());
        _add_to_cache((pname+"ampl").c_str());
        _add_to_cache((pname+"fwhm").c_str());
      }
    } break;
  default:
    break;
  }    
}

void BldSpectrometerHandler::_event    (Pds::TypeId id, const void* payload, const Pds::ClockTime& t)
{
  switch(id.version()) {
  case 0:
    { const Pds::Bld::BldDataSpectrometerV0& d = 
        *reinterpret_cast<const Pds::Bld::BldDataSpectrometerV0*>(payload);

      const uint32_t* h = d.hproj().data();
      for(unsigned i=0; i<1024; i++)
        _entry[0]->content(double(h[i]),i);
      _entry[0]->info(1,EntryWaveform::Normalization);
      _entry[0]->valid(t);

      const uint32_t* v = d.vproj().data();
      for(unsigned i=0; i<256; i++)
        _entry[1]->content(double(v[i]),i);
      _entry[1]->info(1,EntryWaveform::Normalization);
      _entry[1]->valid(t);
    } 
    break;
  case 1:
    { const Pds::Bld::BldDataSpectrometerV1& d = 
        *reinterpret_cast<const Pds::Bld::BldDataSpectrometerV1*>(payload);

      const ndarray<const uint32_t,1> h = d.hproj();
      for(unsigned i=0; i<h.size(); i++)
        _entry[0]->content(double(h[i]),i);
      _entry[0]->info(1,EntryWaveform::Normalization);
      _entry[0]->valid(t);

      if (_index>=0) {
        int index(_index);
        _cache.cache(index++, d.comRaw());
        _cache.cache(index++, d.com());
        _cache.cache(index++, d.integral());
        for(unsigned i=0; i<d.nPeaks() && i<_npeaks; i++) {
          _cache.cache(index++, d.peakPos()[i]);
          _cache.cache(index++, d.peakHeight()[i]);
          _cache.cache(index++, d.FWHM()[i]);
        }
      }
    }
    break;
  default:
    break;
  }
}

void BldSpectrometerHandler::_damaged() 
{
  for(unsigned i=0; i<_nentries; i++) {
    if (_entry[i]) _entry[i]->invalid();
  }
}
