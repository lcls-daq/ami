#ifndef AmiQt_Fit_hh
#define AmiQt_Fit_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/OverlayParent.hh"
#include "ami/qt/Cursors.hh"
#include "ami/service/Semaphore.hh"
#include "ami/data/ConfigureRequest.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class DescEntry;
  class Entry;

  namespace Qt {
    class ChannelDefinition;
    class WaveformDisplay;
    class ScalarPlotDesc;
    class EnvPlot;
    class EnvPost;
    class EnvOverlay;

    class Fit : public QtPWidget,
                public OverlayParent,
                public Cursors {
      Q_OBJECT
    public:
      Fit(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, 
          WaveformDisplay&, QtPWidget*);
      ~Fit();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
                     Ami::ConfigureRequest::Source source);
      void setup_payload(Cds&);
      void update();
      void initialize(const Ami::DescEntry&);
    public:
      void mousePressEvent  (double,double);
      void mouseMoveEvent   (double,double);
      void mouseReleaseEvent(double,double);
    public slots:
      void set_channel   (int); // set the source
      void set_function  (int); // 
      void plot          ();   // configure the plot
      void overlay       ();   // configure the plot
      void remove_plot   (QObject*);
      void add_post      ();
      void add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void remove_overlay(QtOverlay*);
      void grabRange     ();
      void resetRange    ();
    signals:
      void changed();
    private:
      QString _add_post();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;
      WaveformDisplay&  _frame;
      QtPWidget*        _frameParent;
      QComboBox*        _functionBox;
      QComboBox*        _parameterBox;
      ScalarPlotDesc*   _scalar_desc;
      
      QLineEdit*   _xlo;
      QLineEdit*   _xhi;

      QPushButton* _plotB;
      QPushButton* _ovlyB;

      Semaphore           _list_sem;
      std::list<EnvPlot*> _plots;
      std::list<EnvPost*> _posts;
      std::list<EnvOverlay*> _ovls;
    };
  };
};

#endif
