#ifndef AmiQt_PeakFinder_hh
#define AmiQt_PeakFinder_hh

#include <QtGui/QWidget>
#include <QtCore/QString>
#include <QtCore/QStringList>

class QLineEdit;
class QButtonGroup;
class QVBoxLayout;
class QComboBox;

#include "ami/qt/Cursors.hh"

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;

  namespace Qt {
    class ChannelDefinition;
    class ImageScale;
    class PeakPlot;
    class DescImage;
    class ImageDisplay;

    class PeakFinder : public QWidget {
      Q_OBJECT
    public:
      PeakFinder(ChannelDefinition* channels[], unsigned nchannels, ImageDisplay&);
      ~PeakFinder();
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel   (int); // set the source
      void plot          ();   // configure the plot
      void remove_plot   (QObject*);
    signals:
      void changed();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      ImageDisplay&  _frame;
      ImageScale*    _threshold;
 
      std::list<PeakPlot*> _plots;
    };
  };
};

#endif
