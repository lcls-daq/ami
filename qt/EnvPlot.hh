#ifndef AmiQt_EnvPlot_hh
#define AmiQt_EnvPlot_hh

#include "ami/qt/QtPWidget.hh"
#include <QtCore/QString>

#include "ami/data/ConfigureRequest.hh"

#include <list>

class QwtPlot;
class QLabel;

namespace Ami {
  class Cds;
  class DescEntry;
  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class EnvDefinition;
    class QtBase;
    class EnvPlot : public QtPWidget {
      Q_OBJECT
    public:
      EnvPlot(QWidget*,
	      const QString& name,
	      DescEntry*     desc,
	      int            index0,
	      int            index1);
      EnvPlot(QWidget*,const char*&);
      ~EnvPlot();
    private:
      void _layout();
    public:
      void save(char*&) const;
      void load(const char*&);
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
      QLabel*  _counts;
    };
  };
};

#endif
		 
