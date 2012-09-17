#ifndef AmiQt_ImageXYProjection_hh
#define AmiQt_ImageXYProjection_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/CPostParent.hh"
#include "ami/qt/OverlayParent.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QTabWidget;
class QPushButton;

#include <list>

namespace Ami {

  class Cds;
  class Entry;

  namespace Qt {
    class ChannelDefinition;
    class RectangleCursors;
    class ImageFrame;
    class ProjectionPlot;
    class CursorPlot;
    class CursorPost;
    class CursorOverlay;
    class ZoomPlot;
    class XYHistogramPlotDesc;
    class XYProjectionPlotDesc;
    class ScalarPlotDesc;
    class ImageIntegral;
    class ImageContrast;

    class ImageXYProjection : public QtPWidget,
                              public CPostParent,
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
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel(int);
      void update_range();
      void plot        ();   // configure the plot
      void overlay     ();
      void zoom        ();
      void configure_plot();
      void remove_plot (QObject*);
      virtual void setVisible(bool);
      void add_integral_post    ();
      void add_contrast_post    ();
      void plottab_changed(int);
    signals:
      void changed();
    private:
      QString _add_post(const QString&, const char*);
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
      ImageIntegral*        _integral_plot;
      ImageContrast*        _contrast_plot;

      std::list<ProjectionPlot*> _pplots;
      std::list<CursorPlot*>     _cplots;
      std::list<ZoomPlot*>       _zplots;

    public:
      void remove_cursor_post(CursorPost*);
    private:
      std::list<CursorPost*>     _posts;

    public:
      void add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void remove_overlay(QtOverlay*);
    private:
      std::list<CursorOverlay*> _ovls;
    };
  };
};

#endif
