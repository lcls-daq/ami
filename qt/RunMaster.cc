#include "ami/qt/RunMaster.hh"
#include "ami/data/EntryScan.hh"

using namespace Ami::Qt;

static RunMaster* _instance = 0;

RunMaster* RunMaster::instance()
{
  if (!_instance)
    _instance = new RunMaster;
  return _instance;
}

void RunMaster::set_entry(Ami::Entry* e)
{
  _entry = static_cast<Ami::EntryScan*>(e);
}

void RunMaster::reset()
{
  _entry = 0;
}
  
void RunMaster::update   ()
{
  if (_entry && _entry->valid()) {
    double v = _entry->ymean(0);
    if (v>0 && v<100000)
      _run_title = QString("Run %1").arg(v,0);
    else
      _run_title.clear();
  }
}

QString RunMaster::run_title() const
{
  return _run_title;
}

RunMaster::RunMaster() :
  _run_title(""),
  _entry    (0)
{
}

RunMaster::~RunMaster()
{
}
