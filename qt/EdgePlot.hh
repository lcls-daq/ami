#ifndef AmiQt_EdgePlot_hh
#define AmiQt_EdgePlot_hh

#include "ami/qt/QtPWidget.hh"

#include <QtCore/QString>

#include <list>

class QwtPlot;
class QLabel;

namespace Ami {
  class Cds;
  class DescEntry;
  class EdgeFinder;
  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class CursorDefinition;
    class QtBase;
    class EdgePlot : public QtPWidget {
      Q_OBJECT
    public:
      EdgePlot(QWidget*         parent,
	       const QString&   name,
	       unsigned         channel,
	       Ami::EdgeFinder* finder);
      EdgePlot(QWidget*         parent,
	       const char*&     p);
      ~EdgePlot();
    private:
      void _layout();
    public:
      void save(char*& p) const;
      void load(const char*& p);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     const AxisInfo&);
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
      unsigned    _channel;
      Ami::EdgeFinder* _finder;

      unsigned _output_signature;

      QwtPlot* _frame;
      QtBase*  _plot;
      QLabel*  _counts;
    };
  };
};

#endif
		 
