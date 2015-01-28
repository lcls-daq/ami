#include "ami/qt/QtPStack.hh"
#include "ami/qt/QtPWidget.hh"

#include <QtGui/QPushButton>

using namespace Ami::Qt;

static bool _attach=false;

void QtPStack::attach(bool v) { _attach=v; }

QtPStack::QtPStack() : _current(0)
{
}

QtPStack::~QtPStack()
{
}

void QtPStack::add(QPushButton* b,
                   QtPWidget*   w)
{
  if (_attach) {
    addWidget(w);
    w->hide();
    connect(b, SIGNAL(clicked()), w, SIGNAL(opened()));
    connect(w, SIGNAL(opened()), this, SLOT(setPWidget()));
    connect(w, SIGNAL(closed()), this, SLOT(resetPWidget()));
    hide();
  }
  else
    connect(b, SIGNAL(clicked()), w, SLOT(front()));
}

void QtPStack::setVisible(bool v)
{
  v = v && (_current!=0);
  QStackedWidget::setVisible(v);
}

void QtPStack::setPWidget()
{
  QWidget* p = static_cast<QWidget*>(sender());
  setCurrentWidget(_current=p);
  p->show();
  show();
}

void QtPStack::resetPWidget()
{
  _current=0;
  hide();
  updateGeometry();
  resize(minimumSize());
  emit hidden();
}
