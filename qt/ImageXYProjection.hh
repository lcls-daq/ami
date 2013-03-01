#ifndef AmiQt_ImageXYProjection_hh
#define AmiQt_ImageXYProjection_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/OverlayParent.hh"
#include "ami/qt/Rect.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QTabWidget;
class QPushButton;
class QComboBox;

#include <vector>

namespace Ami {

  class Cds;
  class Entry;

  namespace Qt {
    class ChannelDefinition;
    class RectangleCursors;
    class ImageFrame;
    class XYHistogramPlotDesc;
    class XYProjectionPlotDesc;
    class ScalarPlotDesc;
    class ImageFunctions;
    class RectROI;

    class ImageXYProjection : public QtPWidget,
                              public OverlayParent {
      Q_OBJECT
    public:
      ImageXYProjection(QtPWidget* parent,
			ChannelDefinition* channels[], unsigned nchannels, ImageFrame&);
      ~ImageXYProjection();
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
    private:
      RectROI& _roi();
    public slots:
      void set_channel(int);
      void update_range();
      void plot        ();   // configure the plot
      void overlay     ();
      void zoom        ();
      void add_function_post    ();
      void plottab_changed(int);
      void new_roi     ();
      void select_roi  (int);
    signals:
      void changed();
    protected:
      void showEvent(QShowEvent*);
      void hideEvent(QHideEvent*);
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      ImageFrame&       _frame;
      RectangleCursors* _rectangle;

      QLineEdit*    _title;
      QTabWidget*   _plot_tab;
      QPushButton*  _ovlyB;

      XYHistogramPlotDesc*  _histogram_plot;
      XYProjectionPlotDesc* _projection_plot;
      ImageFunctions*       _function_plot;

      std::vector<Rect*>    _rect;
      std::vector<RectROI*> _rois;
      QComboBox*            _roiBox;
      QPushButton*          _roiButton;
    };
  };
};

#endif
