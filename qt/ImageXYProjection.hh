#ifndef AmiQt_ImageXYProjection_hh
#define AmiQt_ImageXYProjection_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/OverlayParent.hh"

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
    class ImageFrame;
    class XYHistogramPlotDesc;
    class XYProjectionPlotDesc;
    class ScalarPlotDesc;
    class ImageFunctions;
    class RectROIDesc;

    class ImageXYProjection : public QtPWidget,
                              public OverlayParent {
      Q_OBJECT
    public:
      ImageXYProjection(QWidget* parent,
			ChannelDefinition* channels[], unsigned nchannels, ImageFrame&);
      ~ImageXYProjection();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void snapshot(const QString&) const;
      void save_plots(const QString&) const;
    public:
      void      add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void      remove_overlay(QtOverlay*);
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void update_range();
      void plot        ();   // configure the plot
      void overlay     ();
      void zoom        ();
      void add_function_post    ();
      void plottab_changed(int);
      void change_channel();
    signals:
      void changed();
    private:
      void _layout(ImageFrame&);
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      QComboBox*            _channelBox;

      QLineEdit*    _title;
      QTabWidget*   _plot_tab;
      QPushButton*  _ovlyB;
      QPushButton*  _plotB;

      RectROIDesc*          _rect;
      XYHistogramPlotDesc*  _histogram_plot;
      XYProjectionPlotDesc* _projection_plot;
      ImageFunctions*       _function_plot;
    };
  };
};

#endif
