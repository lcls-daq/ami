#include <string.h>

#include "ami/data/DescEntry.hh"
#include "ami/data/valgnd.hh"

using namespace Ami;
using Pds::DetInfo;

static const unsigned _weighted_mask = (1<<DescEntry::Scalar) |
                                           (1<<DescEntry::Prof) | 
                                               (1<<DescEntry::Scan);

DescEntry::DescEntry(const char* name,
		     const char* xtitle, 
		     const char* ytitle, 
		     Type type, 
		     unsigned short size,
		     Stat stat,
		     bool isnormalized,
		     bool doaggregate,
                     unsigned options) :
  Desc(name),
  _info(0,DetInfo::NoDetector,0,DetInfo::NoDevice,0),
  _channel(-1),
  _stat(stat),
  _reserved(-1),
  _options(options<<User),
  _type(type),
  _size(size)
{ 
  normalize(isnormalized);
  aggregate(doaggregate);

  strncpy_val(_xtitle, xtitle, TitleSize);
  _xtitle[TitleSize-1] = 0;
  strncpy_val(_ytitle, ytitle, TitleSize);
  _ytitle[TitleSize-1] = 0;
  memset(_zunits, 0, TitleSize);
}

DescEntry::DescEntry(const Pds::DetInfo& info,
		     unsigned channel,
		     const char* name,
		     const char* xtitle, 
		     const char* ytitle, 
		     Type type, 
		     unsigned short size,
		     Stat stat,
		     bool isnormalized,
		     bool doaggregate,
                     unsigned options) :
  Desc(name),
  _info   (info),
  _channel(channel),
  _stat(stat),
  _reserved(-1),
  _options(options<<User),
  _type(type),
  _size(size)
{ 
  normalize(isnormalized);
  aggregate(doaggregate);

  strncpy_val(_xtitle, xtitle, TitleSize);
  _xtitle[TitleSize-1] = 0;
  strncpy_val(_ytitle, ytitle, TitleSize);
  _ytitle[TitleSize-1] = 0;
  memset(_zunits, 0, TitleSize);
}

DescEntry::DescEntry(const Pds::DetInfo& info,
		     unsigned channel,
		     const char* name,
		     const char* xtitle, 
		     const char* ytitle, 
                     const char* zunits,
		     Type type, 
		     unsigned short size,
		     Stat stat,
		     bool isnormalized,
		     bool doaggregate,
                     bool hasPedCalib,
                     bool hasGainCalib, 
                     bool hasRmsCalib,
                     unsigned options) :
  Desc(name),
  _info   (info),
  _channel(channel),
  _stat(stat),
  _reserved(-1),
  _options(options<<User),
  _type(type),
  _size(size)
{ 
  normalize(isnormalized);
  aggregate(doaggregate);

  if (hasPedCalib ) _options |= 1<<CalibMom0;
  if (hasGainCalib) _options |= 1<<CalibMom1;
  if (hasRmsCalib ) _options |= 1<<CalibMom2;

  strncpy_val(_xtitle, xtitle, TitleSize);
  _xtitle[TitleSize-1] = 0;
  strncpy_val(_ytitle, ytitle, TitleSize);
  _ytitle[TitleSize-1] = 0;
  strncpy_val(_zunits, zunits, TitleSize);
  _zunits[TitleSize-1] = 0;
}

DescEntry::DescEntry(const DescEntry& o) :
  Desc    (o),
  _info   (o._info),
  _channel(o._channel),
  _stat   (o._stat),
  _reserved(o._reserved),
  _options(o._options),
  _type   (o._type),
  _size   (o._size)
{ 
  strncpy_val(_xtitle, o._xtitle, TitleSize);
  _xtitle[TitleSize-1] = 0;
  strncpy_val(_ytitle, o._ytitle, TitleSize);
  _ytitle[TitleSize-1] = 0;
  strncpy_val(_zunits, o._zunits, TitleSize);
  _zunits[TitleSize-1] = 0;
}

DescEntry::Type DescEntry::type() const {return Type(_type);}
unsigned short DescEntry::size() const {return _size;}
const char* DescEntry::xtitle() const {return _xtitle;}
const char* DescEntry::ytitle() const {return _ytitle;}
const char* DescEntry::zunits() const {return _zunits;}

const Pds::DetInfo& DescEntry::info() const { return _info; }
unsigned            DescEntry::channel() const { return _channel; }
DescEntry::Stat     DescEntry::stat() const {return Stat(_stat); }

bool DescEntry::isnormalized   () const {return _options&(1<<Normalized);}
bool DescEntry::aggregate      () const {return _options&(1<<Aggregate);}
bool DescEntry::check_refresh  () const {return _options&(1<<CheckRefresh);}
bool DescEntry::force_refresh  () const {return _options&(1<<ForceRefresh);}
bool DescEntry::auto_refresh   () const {return _options&(1<<AutoRefresh);}
bool DescEntry::countmode      () const {return _options&(1<<CountMode);}
bool DescEntry::isweighted_type() const {return ((1<<_type)&_weighted_mask);}
bool DescEntry::hasPedCalib    () const {return _options&(1<<CalibMom0);}
bool DescEntry::hasGainCalib   () const {return _options&(1<<CalibMom1);}
bool DescEntry::hasRmsCalib    () const {return _options&(1<<CalibMom2);}
bool DescEntry::used           () const {return _options&(1<<Used);}
bool DescEntry::recorded       () const {return !(_options&(1<<NotRecorded));}

void DescEntry::_set_opt(bool v,Option opt)
{
  if (v) _options |=  (1<<opt);
  else   _options &= ~(1<<opt);
}

void DescEntry::normalize    (bool v) { _set_opt(v,Normalized); }
void DescEntry::aggregate    (bool v) { _set_opt(v,Aggregate); }
void DescEntry::check_refresh(bool v) { _set_opt(v,CheckRefresh); }
void DescEntry::force_refresh(bool v) { _set_opt(v,ForceRefresh); }
void DescEntry::auto_refresh (bool v) { _set_opt(v,AutoRefresh); }
void DescEntry::countmode    (bool v) { _set_opt(v,CountMode); }
void DescEntry::pedcalib     (bool v) { _set_opt(v,CalibMom0); }
void DescEntry::gaincalib    (bool v) { _set_opt(v,CalibMom1); }
void DescEntry::rmscalib     (bool v) { _set_opt(v,CalibMom2); }
void DescEntry::recorded     (bool v) { _set_opt(!v,NotRecorded); }

void DescEntry::used         (bool v) const {
  if (v) _options |=  (1<<Used);
  else   _options &= ~(1<<Used);
}

void DescEntry::options(unsigned o) {
  _options &= (1<<User)-1;
  _options |= o<<User;
}

static const char* _type_str[] = { "Scalar",
                                   "TH1F",
                                   "TH2F",
                                   "Prof",
                                   "Image",
                                   "Waveform",
                                   "Scan",
                                   "Ref",
                                   "Cache",
                                   "ScalarRange",
                                   "ScalarDRange",
                                   "Prof2D",
                                   NULL };

const char* DescEntry::type_str(Type t) { return _type_str[t]; }
