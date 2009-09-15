#ifndef AmiQt_CursorPlot_hh
#define AmiQt_CursorPlot_hh

#include <QtGui/QWidget>
#include <QtCore/QString>

#include "ami/data/BinMath.hh"

#include <list>

class QwtPlot;

namespace Ami {
  class Cds;
  class DescEntry;
  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class CursorDefinition;
    class QtBase;
    class CursorPlot : public QWidget {
      Q_OBJECT
    public:
      CursorPlot(const QString& name,
		 BinMath*       desc);
      ~CursorPlot();
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
      QString    _name;
      BinMath*   _input;
      
      unsigned _output_signature;

      QwtPlot* _frame;
      QtBase*  _plot;
    };
  };
};

#endif
		 
