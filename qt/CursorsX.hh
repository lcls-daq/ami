#ifndef AmiQt_CursorsX_hh
#define AmiQt_CursorsX_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/CPostParent.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtGui/QLineEdit>
#include <QtGui/QDoubleValidator>

class QVBoxLayout;
class QComboBox;

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
    class CursorPlot;
    class CursorPost;
    class WaveformDisplay;
    class ScalarPlotDesc;

    class CursorLocation : public QLineEdit {
    public:
      CursorLocation() : QLineEdit("0") { new QDoubleValidator(this); }
      ~CursorLocation() {}
    public:
      double value() const { return text().toDouble(); }
    };

    class CursorsX : public QtPWidget,
		     public Cursors,
                     public CPostParent  {
      Q_OBJECT
    public:
      CursorsX(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, 
               WaveformDisplay&, QtPWidget* =0);
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
      void add_post    ();
    signals:
      void changed();
      void grabbed();
    private:
      QString _add_post();
      QString _translate_expr();
    public:
      void mousePressEvent  (double, double);
      void mouseMoveEvent   (double, double);
      void mouseReleaseEvent(double, double);
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      WaveformDisplay&  _frame;
      QtPWidget*        _frameParent;
      QStringList     _names;
      CursorLocation* _new_value;
      QVBoxLayout*    _clayout;

#if 0
      QLineEdit* _expr;
#else
      QComboBox* _expr;
#endif
      QString _expr_text() const;
      QString _translate_expr() const;
      void _expr_setText(const QString&);
      void _expr_save(char*&) const;
      void _expr_load(const char*&);

      ScalarPlotDesc* _scalar_desc;

      std::list<CursorDefinition*> _cursors;
      Ami::AbsOperator* _operator;

      std::list<CursorPlot*> _plots;

    public:
      void remove_cursor_post(CursorPost*);
    private:
      std::list<CursorPost*> _posts;
    };
  };
};

#endif
