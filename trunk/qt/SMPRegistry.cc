#include "ami/qt/SMPRegistry.hh"

using namespace Ami::Qt;

static SMPRegistry* _instance = 0;

SMPRegistry& SMPRegistry::instance()
{
  if (!_instance) _instance = new SMPRegistry;
  return *_instance;
}

void SMPRegistry::nservers(unsigned n) { _nservers=n; }

unsigned SMPRegistry::nservers() { return _nservers; }

SMPRegistry::SMPRegistry() : _nservers(1) {}

SMPRegistry::~SMPRegistry() {}

