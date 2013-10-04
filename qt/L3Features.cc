#include "ami/qt/L3Features.hh"

#include "ami/qt/FeatureRegistry.hh"

using namespace Ami::Qt;

L3Features::L3Features() :
  FeatureTree(&FeatureRegistry::instance(Ami::PostAnalysis))
{
}

L3Features::~L3Features()
{
}

bool L3Features::_valid_entry(const QString& e) const
{
  QStringList v;
  v << "ProcTime"
    << "ProcTimeAcc"
    << "ProcLatency"
    << "EventId"
    << "EventTime"
    << "RunNumber";

  if (v.contains(e))
    return false;

  return FeatureTree::_valid_entry(e);
}

