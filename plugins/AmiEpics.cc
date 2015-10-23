#include "ami/plugins/AmiEpics.hh"
#include "pdsdata/xtc/ClockTime.hh"

//#define DBUG
using Ami_Epics::PVWriter;

Ami::AmiEpics::AmiEpics() : _pv(new PVWriter("ami::clock")) {}
Ami::AmiEpics::~AmiEpics() {}

void Ami::AmiEpics::reset    (Ami::FeatureCache&) 
{
}

void Ami::AmiEpics::clock    (const Pds::ClockTime& clk) 
{
  *(double*)_pv->data() = clk.asDouble();
  _pv->put();
}

void Ami::AmiEpics::configure(const Pds::DetInfo&   src,      // configuration data callback
                              const Pds::TypeId&    type,
                              void*                 payload)
{
}

void Ami::AmiEpics::event    (const Pds::DetInfo&   src,      // event data callback
                              const Pds::TypeId&    type,
                              const Pds::Damage&    damage,
                              void*                 payload)
{
}

void Ami::AmiEpics::clear    ()
{
}

void Ami::AmiEpics::create   (Cds& cds)
{
}

void Ami::AmiEpics::analyze  () {}

const char* Ami::AmiEpics::name() const { return "AmiEpics"; }

bool Ami::AmiEpics::accept () { return true; }


