#ifndef AmiQt_EdgeFinder_hh
#define AmiQt_EdgeFinder_hh

/**
 **  Generate a vector array of hits(edges) for each set of 
 **  edge selection criteria defined.  Plots are created from
 **  the selected set.
 **/

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/OverlayParent.hh"
#include "ami/qt/VAConfigApp.hh"
#include "ami/data/ConfigureRequestor.hh"
#include "ami/data/DescWaveform.hh"

#include <QtCore/QString>

class QComboBox;
class QPushButton;
class QLineEdit;
class QButtonGroup;
class QObject;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;
  class DescEntry;
  class EdgeFinderConfig;

  namespace Qt {
    class ChannelDefinition;
    class VectorArrayDesc;
    class EdgeFinderConfig;
    class EdgeFinderConfigApp;
    class PeakPlot;
    class CursorPlot;
    class CursorPost;
    class CursorOverlay;
    class Cursor;
    class DoubleEdit;
    class WaveformDisplay;

    class EdgeFinder : public QtPWidget,
		       public OverlayParent {
      Q_OBJECT
    public:
      EdgeFinder(QWidget* parent,
		 ChannelDefinition* channels[], unsigned nchannels,
                 WaveformDisplay&, QtPWidget*);
      ~EdgeFinder();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void      add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void      remove_overlay(QtOverlay*);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
      void prototype(const DescEntry&);
    private:
      EdgeFinderConfigApp& _app();
    public slots:
      void set_channel   (int); // set the source
      void plot          ();   // configure the plot
      void overlay       ();   // configure the plot
      void update_interval();
      void new_set       ();
      void select_set    (int);
      void change_channel();
      void update_config ();
    signals:
      void changed();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      QComboBox*                       _channelBox;
      unsigned                         _channel;

      EdgeFinderConfig*                _config;

      QLineEdit*                       _title;

      VectorArrayDesc*                 _analysis_plot;

      std::vector<Ami::EdgeFinderConfig*> _configs;
      std::vector<EdgeFinderConfigApp*>   _apps;

      QComboBox*                       _setBox;
      QPushButton*                     _setButton;
    };

    class EdgeFinderConfigApp : public VAConfigApp {
    public:
      EdgeFinderConfigApp(QWidget* parent, 
			  std::vector<Ami::EdgeFinderConfig*>&,
			  unsigned icfg,
			  unsigned ichan);
      EdgeFinderConfigApp(QWidget* parent, 
			  std::vector<Ami::EdgeFinderConfig*>&,
			  const char*&);
      virtual ~EdgeFinderConfigApp();
    public:
      unsigned config() const { return _icfg; }
    protected:
      Ami::AbsOperator* _op(const char*);
    private:
      std::vector<Ami::EdgeFinderConfig*>&  _config;
      unsigned _icfg;
    };

    class EdgeFinderConfig : public QWidget {
      Q_OBJECT
    public:
      EdgeFinderConfig(QWidget* parent,
                       WaveformDisplay&,
                       QtPWidget*);
      virtual ~EdgeFinderConfig();
    public:
      Ami::EdgeFinderConfig value() const;
      void prototype(const DescEntry&);
      void load(const Ami::EdgeFinderConfig& v);
      void load(const char*& p);
      void save(char*& p) const;
    signals:
      void changed();
    private:
      DoubleEdit* _fraction;
      QButtonGroup* _edge_group;
      DoubleEdit* _deadtime;
      Cursor*   _threshold_value;
      Cursor*   _baseline_value;
      Cursor*   _xlo;
      Cursor*   _xhi;
    };
  };
};

#endif
