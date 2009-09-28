#ifndef AmiQt_EnvPlot_hh
#define AmiQt_EnvPlot_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

#include "ami/data/ConfigureRequest.hh"

#include <list>

class QwtPlot;

namespace Ami {
  class Cds;
  class DescEntry;
  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class EnvDefinition;
    class QtBase;
    class EnvPlot : public QWidget {
      Q_OBJECT
    public:
      EnvPlot(const QString& name,
	      DescEntry*     desc,
	      int            index0,
	      int            index1);
      ~EnvPlot();
    public:
      void configure(char*& p, unsigned input, unsigned& output);
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
      QString    _name;

      DescEntry* _desc;
      int        _index0;
      int        _index1;
      
      unsigned _output_signature;

      QwtPlot* _frame;
      QtBase*  _plot;
    };
  };
};

#endif
		 
