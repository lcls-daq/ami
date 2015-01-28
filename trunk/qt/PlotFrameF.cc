#include "ami/qt/PlotFrameF.hh"

#include <QtGui/QPainter>
#include "qwt_plot_curve.h"

using namespace Ami::Qt;

PlotFrameF::PlotFrameF(QWidget* p) : PlotFrame(p) {}
PlotFrameF::~PlotFrameF() {}

void PlotFrameF::drawItems(QPainter*     p,
                           const QRect&  canvasRect,
                           const QwtScaleMap map[axisCnt],
                           const QwtPlotPrintFilter &pfilter ) const 
{
  QwtPlot::drawItems(p, canvasRect, map, pfilter);

  const QwtPlotItemList& list = itemList();
  int row=0;
  for(int i=0; i<list.size(); i++) {
    QwtPlotCurve* c=dynamic_cast<QwtPlotCurve*>(list[i]);
    if (c && c->title().text().contains('['))
      { row++; break; }
  }

  if (row) {
    row=0;
    int width = canvasRect.width();
    QPainter& painter = *p;
    for(int i=0; i<list.size(); i++) {
      QwtPlotCurve* c = dynamic_cast<QwtPlotCurve*>(list[i]);
      if (c && c->title().text().contains('[')) {
        painter.setPen  (c->pen().color());
        const QString& title = c->title().text();
        if (width > 20) {
          painter.drawText(10, 18*row, width-10, 18, ::Qt::AlignLeft, 
                           title.mid(0,title.indexOf('#')));
          row++;
        }
      }
    }
  }
}
