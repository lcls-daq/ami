#include "ami/qt/RunMaster.hh"
#include "ami/data/EntryScalar.hh"

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
  _entry = static_cast<Ami::EntryScalar*>(e);
}

void RunMaster::reset()
{
  _entry = 0;
}
  
void RunMaster::update   ()
{
  if (_entry && _entry->valid()) {
    double n = _entry->entries()-_prev_n;
    if (n > 0) {
      double v = (_entry->sum() - _prev_s)/n;
      if (v>0 && v<100000)
        _run_title = QString("Run %1").arg(_entry->mean(),0);
    }
    _prev_n = _entry->entries();
    _prev_s = _entry->sum();
  }
}

QString RunMaster::run_title() const
{
  return _run_title;
}

RunMaster::RunMaster() :
  _run_title(""),
  _entry    (0),
  _prev_s   (0),
  _prev_n   (0)
{
}

RunMaster::~RunMaster()
{
}
