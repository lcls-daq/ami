#ifndef AmiQt_EdgeFinder_hh
#define AmiQt_EdgeFinder_hh

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
    class AxisArray;
    class ChannelDefinition;
    class EdgeCursor;
    class EdgePlot;
    class DescTH1F;
    class DescProf;
    class DescChart;
    class Display;

    class EdgeFinder : public QWidget {
      Q_OBJECT
    public:
      EdgeFinder(ChannelDefinition* channels[], unsigned nchannels, Display&);
      ~EdgeFinder();
    public:
      Ami::AbsOperator* math() const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels);
      void setup_payload(Cds&);
      void update();
    public slots:
      void set_channel   (int); // set the source
      void plot          ();   // configure the plot
      void load          ();
      void save          ();
      void remove_plot   (QObject*);
    signals:
      void changed();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;

      Display&  _frame;
      EdgeCursor* _baseline;
      EdgeCursor* _threshold;
 
      QLineEdit* _title;
      DescTH1F*  _hist;
      Ami::AbsOperator* _operator;
      bool _grab_baseline;
      std::list<EdgePlot*> _plots;
    };
  };
};

#endif
