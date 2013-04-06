#include "ami/qt/RecvCalculator.hh"

#include "ami/service/ConnectionManager.hh"

using namespace Ami::Qt;

RecvCalculator::RecvCalculator(ConnectionManager& m) : 
  QLabel("-.-- MB/s"),
  _m(m) 
{
  connect(this, SIGNAL(changed(QString)), this, SLOT(change(QString)));
}

RecvCalculator::~RecvCalculator() {}

void RecvCalculator::update() 
{
  unsigned v = _m.receive_bytes(); 
  QString t;
  if (v > 1e8)
    t = QString("%1 GB/s").arg(double(v)*1.e-9,0,'f',1);
  else if (v > 1e5)
    t = QString("%1 MB/s").arg(double(v)*1.e-6,0,'f',1);
  else if (v > 1e2)
    t = QString("%1 kB/s").arg(double(v)*1.e-3,0,'f',1);
  else
    t = QString("%1  B/s").arg(double(v),0);

  emit changed(t);
}

void RecvCalculator::change(QString t)
{
  setText(t);
}
