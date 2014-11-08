#include "ami/qt/CpuCalculator.hh"

#include "ami/qt/SMPRegistry.hh"

#include <QtCore/QString>

using namespace Ami::Qt;

CpuCalculator::CpuCalculator() 
{
}

CpuCalculator::~CpuCalculator() {}

void CpuCalculator::reset()
{
  _cputime=0;
  clock_gettime(CLOCK_REALTIME,&_last);
}

void CpuCalculator::update() {
  if (_entry && _entry->valid()) {
    timespec tv;
    clock_gettime(CLOCK_REALTIME,&tv);
    double realtime = double(tv.tv_sec - _last.tv_sec)+1.e-9*(double(tv.tv_nsec)-double(_last.tv_nsec));
    double cputime  = (_entry->sum() - _cputime);
    double q        = 100.*cputime/(realtime*double(SMPRegistry::instance().nservers()));
    _last           = tv;
    _cputime        = _entry->sum();
    emit changed(QString("%1\%").arg(QString::number(q,'f',1)));
  } else {
    emit changed(QString("."));
  }
}
