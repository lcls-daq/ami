#ifndef AmiQt_ImageXYProjection_hh
#define AmiQt_ImageXYProjection_hh

#include <QtGui/QWidget>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QButtonGroup;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;

  namespace Qt {
    class ChannelDefinition;
    class RectangleCursors;
    class ImageFrame;
    class ProjectionPlot;
    class ZoomPlot;

    class ImageXYProjection : public QWidget {
      Q_OBJECT
    public:
      ImageXYProjection(ChannelDefinition* channels[], unsigned nchannels, ImageFrame&);
      ~ImageXYProjection();
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel(int); // set the source
      void plot        ();   // configure the plot
      void zoom        ();
      void configure_plot();
      void remove_plot (QObject*);
      virtual void setVisible(bool);
    signals:
      void changed();
    private:
      void _set_cursor  (double, double);
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      ImageFrame&       _frame;
      RectangleCursors* _rectangle;

      QLineEdit*    _title;
      QButtonGroup* _axis;
      QButtonGroup* _norm;

      Ami::AbsOperator* _operator;

      std::list<ProjectionPlot*> _pplots;
      std::list<ZoomPlot*>       _zplots;
    };
  };
};

#endif
