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
class QCheckBox;
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

    class EdgeFinder : public QtPWidget,
		       public OverlayParent {
      Q_OBJECT
    public:
      EdgeFinder(QWidget* parent,
		 ChannelDefinition* channels[], unsigned nchannels);
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
			  const QString&, 
			  unsigned i, 
			  const Ami::EdgeFinderConfig& c);
      virtual ~EdgeFinderConfigApp();
    protected:
      Ami::AbsOperator* _op(const char*);
    private:
      const Ami::EdgeFinderConfig&  _config;
    };

    class EdgeFinderConfig : public QWidget {
      Q_OBJECT
    public:
      EdgeFinderConfig(QWidget* parent);
      virtual ~EdgeFinderConfig();
    public:
      Ami::EdgeFinderConfig value() const;
      void load(const Ami::EdgeFinderConfig& v);
      void load(const char*& p);
      void save(char*& p) const;
    signals:
      void changed();
    private:
      QLineEdit* _fraction;
      QCheckBox* _leading_edge;
      QLineEdit* _deadtime;
      QLineEdit* _threshold_value;
      QLineEdit* _baseline_value;
    };
  };
};

#endif
