#ifndef AmiQt_CurveFit_hh
#define AmiQt_CurveFit_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/CFPostParent.hh"
#include "ami/qt/OverlayParent.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QComboBox;
class QLabel;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class DescEntry;
  class Entry;

  namespace Qt {
    class AxisInfo;
    class ChannelDefinition;
    class WaveformDisplay;
    class ScalarPlotDesc;
    class CurveFitPlot;
    class CurveFitPost;
    class CurveFitOverlay;

    class CurveFit : public QtPWidget,
                     public CFPostParent,
                     public OverlayParent {
      Q_OBJECT
    public:
      CurveFit(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels, WaveformDisplay&);
      ~CurveFit();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
      void initialize(const Ami::DescEntry&);
    public slots:
      void load_file();
      void set_channel   (int); // set the source
      void plot          ();   // configure the plot
      void overlay       ();   // configure the plot
      void remove_plot   (QObject*);
      void add_post      ();
    signals:
      void changed();
    private:
      QString _add_post();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;
      WaveformDisplay&  _frame;
      QString _fname;
      QLabel* _file;
      QComboBox* _outBox;
      ScalarPlotDesc* _scalar_desc;
      std::list<CurveFitPlot*> _plots;

    public:
      void remove_curvefit_post(CurveFitPost*);
    private:
      std::list<CurveFitPost*> _posts;

    public:
      void add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void remove_overlay(QtOverlay*);
    private:
      std::list<CurveFitOverlay*> _ovls;

      static char *_opname[];
    };
  };
};

#endif
