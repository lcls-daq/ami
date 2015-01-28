#ifndef AmiQt_Droplet_hh
#define AmiQt_Droplet_hh

/**
 **  Generate a vector array of hits(photons) for each set of 
 **  droplet selection criteria defined.  Plots are created from
 **  the selected set.
 **
 **  Eventually, this could be generalized to inserting a vector array
 **  into the PostAnalysis feature set of creating plots from there.
 **/

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/OverlayParent.hh"
#include "ami/qt/VAConfigApp.hh"
#include "ami/data/ConfigureRequestor.hh"
#include "ami/data/DescImage.hh"

#include <QtCore/QString>

class QComboBox;
class QPushButton;
class QLineEdit;
class QTabWidget;
class QObject;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;
  class DescEntry;
  class DropletConfig;
  class VAPlot;

  namespace Qt {
    class ChannelDefinition;
    class MapPlotDesc;
    class VectorArrayDesc;
    class DropletConfig;
    class DropletConfigApp;
    class PeakPlot;
    class CursorPlot;
    class ZoomPlot;
    class CursorPost;
    class CursorOverlay;
    class RectROI;
    class RectROIDesc;
    class ImageFrame;

    class Droplet : public QtPWidget,
		    public OverlayParent {
      Q_OBJECT
    public:
      Droplet(QWidget* parent,
	      ChannelDefinition* channels[], unsigned nchannels,
	      ImageFrame&);
      ~Droplet();
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
      DropletConfigApp& _app();
    public slots:
      void set_channel   (int); // set the source
      void plot          ();   // configure the plot
      void post          ();   // configure the plot
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
      Ami::DescImage                   _prototype;

      DropletConfig*                   _config;
      RectROIDesc*                     _rect;

      QLineEdit*                       _title;
      QTabWidget*                      _plot_tab;

      MapPlotDesc*                     _map_plot;
      VectorArrayDesc*                 _analysis_plot;

      std::vector<Ami::DropletConfig*> _configs;
      std::vector<RectROI*>            _rois;
      std::vector<DropletConfigApp*>   _apps;

      QComboBox*                       _setBox;
      QPushButton*                     _setButton;

    };

    class DropletConfigApp : public VAConfigApp {
    public:
      DropletConfigApp(QWidget* parent, 
		       const std::vector<Ami::DropletConfig*>&,
		       const std::vector<RectROI*>&,
		       unsigned iconfig,
		       unsigned iroi,
		       unsigned ichannel);
      DropletConfigApp(QWidget* parent,
		       const std::vector<Ami::DropletConfig*>&,
		       const std::vector<RectROI*>&,
		       const char*&);
      virtual ~DropletConfigApp();
    public:
      unsigned config() const { return _icfg; }
      unsigned roi   () const { return _iroi; }
    protected:
      Ami::AbsOperator* _op(const char*);
    private:
      const std::vector<Ami::DropletConfig*> _config;
      const std::vector<RectROI*>            _roi;
      unsigned _icfg;
      unsigned _iroi;
    };

    class DropletConfig : public QWidget {
      Q_OBJECT
    public:
      DropletConfig(QWidget* parent);
      virtual ~DropletConfig();
    public:
      Ami::DropletConfig value() const;
      void load(const Ami::DropletConfig& v);
      void load(const char*& p);
      void save(char*& p) const;
    signals:
      void changed();
    private:
      QLineEdit* _seed_thr;
      QLineEdit* _nbor_thr;
      QLineEdit* _esum_min;
      QLineEdit* _esum_max;
      QLineEdit* _npix_min;
      QLineEdit* _npix_max;
    };
  };
};

#endif
