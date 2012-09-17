#ifndef AmiQt_ImageRPhiProjection_hh
#define AmiQt_ImageRPhiProjection_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/qt/CPostParent.hh"
#include "ami/qt/OverlayParent.hh"

#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QPushButton;
//class QCheckBox;
class QTabWidget;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;

  namespace Qt {
    class AnnulusCursors;
    class ChannelDefinition;
    class ImageFrame;
    class ProjectionPlot;
    class CursorPlot;
    class CursorPost;
    class CursorOverlay;
    class RPhiProjectionPlotDesc;
    class ImageIntegral;
    class ImageContrast;

    class ImageRPhiProjection : public QtPWidget,
                                public CPostParent,
                                public OverlayParent {
      Q_OBJECT
    public:
      ImageRPhiProjection(QtPWidget* parent,
			  ChannelDefinition* channels[], unsigned nchannels, ImageFrame&);
      ~ImageRPhiProjection();
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
      void set_channel(int); // set the source
      void plot        ();   // configure the plot
      void overlay     ();   // configure the plot
      void remove_plot (QObject*);
      virtual void setVisible(bool);
      void update_range();
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

      ImageFrame&  _frame;
      AnnulusCursors* _annulus;

      QLineEdit* _title;

      QTabWidget*   _plot_tab;
      QPushButton*  _ovlyB;

      RPhiProjectionPlotDesc* _projection_plot;
      ImageIntegral*          _integral_plot;
      ImageContrast*          _contrast_plot;

      std::list<ProjectionPlot*> _pplots;
      std::list<CursorPlot*>     _cplots;

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
