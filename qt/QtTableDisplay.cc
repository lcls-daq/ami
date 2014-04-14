#include "ami/qt/QtTableDisplay.hh"
#include "ami/qt/QtTable.hh"

#include <QtGui/QVBoxLayout>

using namespace Ami::Qt;

static QtTableDisplay* _instance = 0;

QtTableDisplay::QtTableDisplay() :
  QWidget(0)
{
  //  setAttribute(::Qt::WA_DeleteOnClose, true);
  QVBoxLayout* l = new QVBoxLayout;
  l->setSpacing(0);
  setLayout(l);
  show();
}

QtTableDisplay::~QtTableDisplay()
{
  _instance = 0;
}

void QtTableDisplay::add(QtTable& q)
{
  static_cast<QVBoxLayout*>(layout())->addWidget(&q);
}

void QtTableDisplay::update()
{
  _size_changed = true;
  updateGeometry();
  resize(minimumWidth(),minimumHeight());
}

void QtTableDisplay::paintEvent(QPaintEvent* e)
{
  if (_size_changed) {
    resize(minimumWidth(),minimumHeight());
    _size_changed = false;
  }
  QWidget::paintEvent(e);
}

QtTableDisplay* QtTableDisplay::instance()
{
  if (!_instance)
    _instance = new QtTableDisplay;

  return _instance;
}
