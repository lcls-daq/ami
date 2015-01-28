#include "DetectorListItem.hh"

#include <QtGui/QBrush>
#include <QtGui/QColor>

using namespace Ami::Qt;

DetectorListItem::DetectorListItem(QListWidget*        parent,
				   const QString&      dlabel,
				   const Pds::DetInfo& dinfo, 
				   unsigned            dchannel) :
  QListWidgetItem(dlabel, parent),
  info           (dinfo),
  channel        (dchannel) 
{
  setTextAlignment(::Qt::AlignHCenter);
}

DetectorListItem::~DetectorListItem() 
{
}

void DetectorListItem::setState(DetectorListItem::State state)
{
  switch(state) {
  case Warning:
    setBackground(QBrush(::Qt::yellow)); break;
  case OK:
  default:
    setBackground(QBrush(::Qt::color0)); break;
  }
}
