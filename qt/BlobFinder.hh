#ifndef AmiQt_BlobFinder_hh
#define AmiQt_BlobFinder_hh

#include "ami/qt/QtPWidget.hh"

#include <QtCore/QString>

class QLineEdit;
class QCheckBox;

#include <list>

namespace Ami {

  class AbsOperator;
  class Cds;
  class Entry;
  class DescEntry;

  namespace Qt {
    class ChannelDefinition;
    class ImageScale;
    class PeakPlot;
    class DescImage;
    class ImageFrame;
    class RectangleCursors;

    class BlobFinder : public QtPWidget {
      Q_OBJECT
    public:
      BlobFinder(QtPWidget* parent,
		 ChannelDefinition* channels[], unsigned nchannels, ImageFrame&);
      ~BlobFinder();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
      void prototype(const DescEntry&);
    public slots:
      void set_channel   (int); // set the source
      void plot          ();   // configure the plot
      void remove_plot   (QObject*);
      virtual void setVisible(bool);
    signals:
      void changed();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      ImageFrame&       _frame;
      RectangleCursors* _rectangle;
      ImageScale*       _threshold;
      QLineEdit*        _cluster_size;
      QCheckBox*        _accumulate;

      std::list<PeakPlot*> _plots;
    };
  };
};

#endif
