#ifndef AmiQt_CursorsX_hh
#define AmiQt_CursorsX_hh

#include <QtGui/QWidget>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QButtonGroup;
class QVBoxLayout;
class QComboBox;

#include "ami/qt/Cursors.hh"
#include "ami/data/ConfigureRequest.hh"

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;

  namespace Qt {
    class AxisArray;
    class ChannelDefinition;
    class CursorDefinition;
    class CursorLocation;
    class CursorPlot;
    class DescTH1F;
    class DescProf;
    class DescChart;
    class WaveformDisplay;

    class CursorsX : public QWidget,
		     public Cursors {
      Q_OBJECT
    public:
      CursorsX(ChannelDefinition* channels[], unsigned nchannels, WaveformDisplay&);
      ~CursorsX();
    public:
      Ami::AbsOperator* math() const;
    public:
      void remove(CursorDefinition&);
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     ConfigureRequest::Source);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel(int); // set the source
      void calc        ();
      void add_cursor  ();
      void hide_cursors();
      void plot        ();   // configure the plot
      void load        ();
      void save        ();
      void remove_plot (QObject*);
      void grab_cursorx();
      void change_features();
    signals:
      void changed();
      void grabbed();
    private:
      void _set_cursor  (double, double);
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
      DescProf*  _vBld;
      DescProf*  _vFeature;
      QComboBox* _features;

      std::list<CursorDefinition*> _cursors;
      Ami::AbsOperator* _operator;

      std::list<CursorPlot*> _plots;
    };
  };
};

#endif
