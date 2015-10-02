#include "QtPWidget.hh"

#include <QtCore/QPoint>
#include <QtGui/QCloseEvent>
#include <QtGui/QStackedWidget>
#include <QtGui/QVBoxLayout>
#include <QtGui/QPixmap>

#include <stdio.h>

using namespace Ami::Qt;

QtPWidget::QtPWidget() :
  QWidget(0),
  _parent(0)
{
}

QtPWidget::QtPWidget(int) : 
  QWidget(0),
  _parent(0),
  _initial_hide(false)
{
}

QtPWidget::QtPWidget(QWidget* parent) : 
  QWidget(0),
  _parent(parent),
  _initial_hide(false)
{
}

QtPWidget::~QtPWidget() 
{
}

void QtPWidget::save(char*& p) const
{
  XML_insert(p, "int", "x", QtPersistent::insert(p,pos().x()) );
  XML_insert(p, "int", "y", QtPersistent::insert(p,pos().y()) );

  XML_insert(p, "int", "width", QtPersistent::insert(p,size().width()) );
  XML_insert(p, "int", "height",QtPersistent::insert(p,size().height()) );

  XML_insert(p, "bool", "visible", QtPersistent::insert(p,isVisible()) );

#if 0
  if (isVisible())
    printf("QtP save %d,%d %d,%d %c\n",
	   pos().x(),pos().y(),
	   size().width(),size().height(),
	   isVisible()?'t':'f');
#endif
}

void QtPWidget::load(const char*& p)
{
  QPoint r;
  QSize s;
  bool v = false;

  XML_iterate_open(p,tag)
    if (tag.name == "x")
      r.setX(QtPersistent::extract_i(p));
    else if (tag.name == "y")
      r.setY(QtPersistent::extract_i(p));
    else if (tag.name == "width")
      s.setWidth (QtPersistent::extract_i(p));
    else if (tag.name == "height")
      s.setHeight(QtPersistent::extract_i(p));
    else if (tag.name == "visible")
      v=QtPersistent::extract_b(p);
  XML_iterate_close(QtPWidget,tag);

  if (v) {
    move  (r);
    resize(s);
    //    printf("QtP load %d,%d %d,%d %c\n",r.x(),r.y(),s.width(),s.height(),v?'t':'f');
  }

  setVisible(v);
  _initial_hide = !v;
}

void QtPWidget::snapshot(const QString& dir) const
{
}

void QtPWidget::_snapshot(const QString& fname) const
{
  QPixmap pixmap(QWidget::size());
  QtPWidget* cthis = const_cast<QtPWidget*>(this);
  cthis->render(&pixmap);
  pixmap.toImage().save(fname);
  printf("snapshot %s\n",qPrintable(fname));
}

void QtPWidget::front()
{
  //  if (_parent)
  //    move(_parent->pos());

  if (!isVisible()) {
    emit opened();
    show();
  }
  else {
    raise();
    activateWindow();
  }
}

void QtPWidget::closeEvent(QCloseEvent* event)
{
  event->accept();
  emit closed(this);
}
