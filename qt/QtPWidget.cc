#include "QtPWidget.hh"

#include <QtCore/QPoint>

using namespace Ami::Qt;

QtPWidget::QtPWidget(QWidget* parent) : 
  //  QWidget(parent,::Qt::Window) 
  QWidget(0)
{
}

QtPWidget::~QtPWidget() 
{
}

void QtPWidget::save(char*& p) const
{
  QtPersistent::insert(p,pos().x());
  QtPersistent::insert(p,pos().y());
  QtPersistent::insert(p,isVisible() ? 1:0);
}

void QtPWidget::load(const char*& p)
{
  int x,y,v;
  x=QtPersistent::extract_i(p);
  y=QtPersistent::extract_i(p);
  v=QtPersistent::extract_i(p);

  setVisible(v ? true : false);
  move(x,y);

//   printf("QtP load %d,%d %c\n",x,y,v?'t':'f');
}
