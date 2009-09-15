#ifndef AmiQt_EdgePlot_hh
#define AmiQt_EdgePlot_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

#include <list>

class QwtPlot;

namespace Ami {
  class Cds;
  class DescEntry;
  class EdgeFinder;
  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class CursorDefinition;
    class QtBase;
    class EdgePlot : public QWidget {
      Q_OBJECT
    public:
      EdgePlot(const QString&   name,
	       Ami::EdgeFinder* finder);
      ~EdgePlot();
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisArray&);
      void setup_payload(Cds&);
      void update();
    signals:
      void redraw();
    public slots:
      void save_data();
      void set_plot_title();
      void set_xaxis_title();
      void set_yaxis_title();
    private:
      QString     _name;

      Ami::EdgeFinder* _finder;

      unsigned _output_signature;

      QwtPlot* _frame;
      QtBase*  _plot;
    };
  };
};

#endif
		 
