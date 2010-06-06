#include "ami/app/SummaryAnalysis.hh"

#include "ami/data/Cds.hh"
#include "ami/data/EntryScalar.hh"
#include "ami/data/DescScalar.hh"

#include "pdsdata/xtc/ClockTime.hh"

using namespace Ami;

static EntryScalar* _seconds = 0;
static EntryScalar* _minutes = 0;
static Cds* _cds = 0;
static Pds::ClockTime _clk;

SummaryAnalysis::SummaryAnalysis() {}
SummaryAnalysis::~SummaryAnalysis() {}

void SummaryAnalysis::reset    () {}
void SummaryAnalysis::clock    (const Pds::ClockTime& clk) { _clk=clk; }
void SummaryAnalysis::configure(const Pds::Src&       src,
				const Pds::TypeId&    type,
				void*                 payload) {}
void SummaryAnalysis::event    (const Pds::Src&       src,
				const Pds::TypeId&    type,
				void*                 payload) {}

void SummaryAnalysis::clear    () 
{
  if (_seconds) {
    _cds->remove(_seconds); 
    _cds = 0;
  }
}

void SummaryAnalysis::create   (Cds& cds)
{
  _seconds = new EntryScalar(DescScalar("Seconds","Seconds"));
  _minutes = new EntryScalar(DescScalar("Minutes","Minutes"));
  _cds = &cds; 
  cds.add(_seconds);
  cds.add(_minutes);
}

void SummaryAnalysis::analyze  () 
{
  if (_cds) {
    _seconds->valid(_clk);
    _seconds->addcontent(_clk.seconds()%60); 
    _minutes->valid(_clk);
    _minutes->addcontent((_clk.seconds()/60)%60); 
  }
}


static SummaryAnalysis* _instance = 0;

SummaryAnalysis& SummaryAnalysis::instance() 
{
  if (!_instance)
    _instance = new SummaryAnalysis;
  return *_instance; 
}
