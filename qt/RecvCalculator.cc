#include "ami/qt/RecvCalculator.hh"

#include "ami/service/ConnectionManager.hh"

using namespace Ami::Qt;

RecvCalculator::RecvCalculator(ConnectionManager& m) : 
  QLabel("-.-- MB/s"),
  _m(m) 
{
  clock_gettime(CLOCK_REALTIME,&_last);
  connect(this, SIGNAL(changed(QString)), this, SLOT(change(QString)));
}

RecvCalculator::~RecvCalculator() {}

void RecvCalculator::update() 
{
  unsigned q = _m.receive_bytes(); 

  struct timespec tv;
  clock_gettime(CLOCK_REALTIME,&tv);
  double d = double(tv.tv_sec-_last.tv_sec)+1.e-9*(double(tv.tv_nsec)-double(_last.tv_nsec));
  _last = tv;

  double v = q/d;

  QString t;
  if (v > 1.e8)
    t = QString("%1 GB/s").arg(v*1.e-9,0,'f',1);
  else if (v > 1.e5)
    t = QString("%1 MB/s").arg(v*1.e-6,0,'f',1);
  else if (v > 1.e2)
    t = QString("%1 kB/s").arg(v*1.e-3,0,'f',1);
  else
    t = QString("%1  B/s").arg(v,0);

  emit changed(t);
}

void RecvCalculator::change(QString t)
{
  setText(t);
}
