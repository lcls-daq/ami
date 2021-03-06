#ifndef AmiQt_PeakFinder_hh
#define AmiQt_PeakFinder_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/service/Semaphore.hh"

#include <QtCore/QString>

class QCheckBox;
class QComboBox;
class QPushButton;
class QButtonGroup;
class QLineEdit;
class QLabel;

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
    class ZoomPlot;
    class DescImage;
    class QAggSelect;

    class PeakFinder : public QtPWidget {
      Q_OBJECT
    public:
      PeakFinder(QWidget* parent,
		 ChannelDefinition* channels[], unsigned nchannels);
      ~PeakFinder();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void snapshot(const QString&) const;
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
      void update_agg();
      void change_channel();
    signals:
      void changed();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      QComboBox*     _channelBox;
      ImageScale*    _threshold;
      QButtonGroup*  _proc_grp;
      QCheckBox*     _center_only;
      QCheckBox*     _accumulate;
      QAggSelect*    _agg_select;
      QPushButton*   _plotB;

      Semaphore            _list_sem;
      std::list<PeakPlot*> _plots;
      std::list<ZoomPlot*> _zplots;
    };
  };
};

#endif
