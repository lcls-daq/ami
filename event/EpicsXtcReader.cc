#include "EpicsXtcReader.hh"

#include "pdsdata/xtc/Dgram.hh"
#include "pdsdata/xtc/Level.hh"
#include "pdsdata/xtc/Xtc.hh"
#include "pdsdata/xtc/ProcInfo.hh"

#include <stdio.h>

using namespace Ami;

EpicsXtcReader::EpicsXtcReader(FeatureCache& f)  : 
  EventHandler(Pds::ProcInfo(Pds::Level::Observer,0,0),
	       Pds::TypeId::NumberOf,
	       Pds::TypeId::NumberOf),  // expect no configure
  _cache(f),
  _index(-1)
{
}

EpicsXtcReader::~EpicsXtcReader()
{
}

//  no configure data will appear from BLD
void   EpicsXtcReader::_configure(const void* payload)
{
}

void   EpicsXtcReader::_event    (const void* payload)
{
}

void   EpicsXtcReader::_damaged  ()
{
}

//  No Entry data
unsigned     EpicsXtcReader::nentries() const { return 0; }
const Entry* EpicsXtcReader::entry   (unsigned) const { return 0; }
void         EpicsXtcReader::reset   () 
{
}
