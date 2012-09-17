#include "ami/qt/QtUtils.hh"

#include <QtGui/QLayout>
#include <QtGui/QLayoutItem>
#include <QtGui/QWidget>

using namespace Ami::Qt;

void QtUtils::setChildrenVisible(QLayout* l, bool v)
{
  for(int i=0; i<l->count(); i++) {
    QLayoutItem* item = l->itemAt(i);
    if (item->widget())
      item->widget()->setVisible(v);
    else if (item->layout())
      setChildrenVisible(item->layout(), v);
  }
}
