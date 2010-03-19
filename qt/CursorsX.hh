#ifndef AmiQt_CursorsX_hh
#define AmiQt_CursorsX_hh

#include "ami/qt/QtPWidget.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QButtonGroup;
class QVBoxLayout;

#include "ami/qt/Cursors.hh"
#include "ami/data/ConfigureRequest.hh"

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class DescEntry;
  class Entry;

  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class CursorDefinition;
    class CursorLocation;
    class CursorPlot;
    class DescTH1F;
    class DescProf;
    class DescScan;
    class DescChart;
    class WaveformDisplay;

    class CursorsX : public QtPWidget,
		     public Cursors {
      Q_OBJECT
    public:
      CursorsX(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, WaveformDisplay&);
      ~CursorsX();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      Ami::AbsOperator* math() const;
    public:
      void remove(CursorDefinition&);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     ConfigureRequest::Source);
      void setup_payload(Cds&);
      void update();
      void initialize(const Ami::DescEntry&);
    public slots:
      void set_channel(int); // set the source
      void calc        ();
      void add_cursor  ();
      void hide_cursors();
      void plot        ();   // configure the plot
      void remove_plot (QObject*);
      void grab_cursorx();
    signals:
      void changed();
      void grabbed();
    public:
      void mousePressEvent  (double, double);
      void mouseReleaseEvent  (double, double);
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      WaveformDisplay&  _frame;
      QStringList     _names;
      CursorLocation* _new_value;
      QVBoxLayout*    _clayout;

      QLineEdit* _expr;

      QLineEdit* _title;
      QButtonGroup* _plot_grp;
      DescTH1F*  _hist;
      DescChart* _vTime;
      DescProf*  _vFeature;
      DescScan*  _vScan;

      std::list<CursorDefinition*> _cursors;
      Ami::AbsOperator* _operator;

      std::list<CursorPlot*> _plots;
    };
  };
};

#endif
