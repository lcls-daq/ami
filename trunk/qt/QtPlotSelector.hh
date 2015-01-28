#ifndef AmiQt_QtPlotSelector_hh
#define AmiQt_QtPlotSelector_hh

#include "ami/data/DescEntry.hh"

#include <QtGui/QDialog>

class QListWidgetItem;

namespace Ami {
  class DescEntry;
  namespace Qt {
    class OverlayParent;
    class QtPlot;
    class SharedData;
    class QtPlotSelector : public QDialog {
      Q_OBJECT
    public:
      QtPlotSelector(QWidget&      ,
                     OverlayParent&,
                     DescEntry*    ,
                     SharedData*   =0);
      ~QtPlotSelector();
    public:
      void plot_selected(QtPlot*);
      Ami::DescEntry::Type type() const;
    public slots:
      void plot_selected(QListWidgetItem*);
      void grab_plot    ();
    private:
      OverlayParent& _src;
      DescEntry*     _desc;
      SharedData*    _shared;
    };
  };
};

#endif
