#ifndef AmiQt_PlotFrameF_hh
#define AmiQt_PlotFrameF_hh

#include "ami/qt/PlotFrame.hh"

namespace Ami {
  namespace Qt {
    class PlotFrameF : public PlotFrame {
    public:
      PlotFrameF(QWidget*);
      ~PlotFrameF();
    public:
      void drawItems(QPainter*     p,
                     const QRect&  canvasRect,
                     const QwtScaleMap map[axisCnt],
                     const QwtPlotPrintFilter &pfilter ) const;
    };
  };
};

#endif
