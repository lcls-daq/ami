#include "ami/qt/PostAnalysis.hh"

using namespace Ami::Qt;

static PostAnalysis* _instance = 0;

PostAnalysis* PostAnalysis::instance() { return _instance; }

PostAnalysis::PostAnalysis() { _instance=this; }
