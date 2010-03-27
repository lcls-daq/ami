#include "QtPWidget.hh"

#include <QtCore/QPoint>

using namespace Ami::Qt;

QtPWidget::QtPWidget() :
  QWidget(0)
{
}

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

  QtPersistent::insert(p,size().width());
  QtPersistent::insert(p,size().height());

  QtPersistent::insert(p,isVisible());

  if (isVisible())
    printf("QtP save %d,%d %d,%d %c\n",
	   pos().x(),pos().y(),
	   size().width(),size().height(),
	   isVisible()?'t':'f');
}

void QtPWidget::load(const char*& p)
{
  QPoint r;
  r.setX(QtPersistent::extract_i(p));
  r.setY(QtPersistent::extract_i(p));

  QSize s;
  s.setWidth (QtPersistent::extract_i(p));
  s.setHeight(QtPersistent::extract_i(p));

  bool v=QtPersistent::extract_b(p);

  setVisible(v);
  if (v) {
    move  (r);
    resize(s);
    printf("QtP load %d,%d %d,%d %c\n",r.x(),r.y(),s.width(),s.height(),v?'t':'f');
  }
}
