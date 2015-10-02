#ifndef AmiQt_FFT_hh
#define AmiQt_FFT_hh

#include "ami/qt/QtPWidget.hh"

#include "ami/data/ConfigureRequest.hh"
#include "ami/service/Semaphore.hh"

class QComboBox;

namespace Ami {
  class Cds;

  namespace Qt {
    class ChannelDefinition;
    class ProjectionPlot;

    class FFT : public QtPWidget {
      Q_OBJECT
    public:
      FFT(QWidget* parent, ChannelDefinition* channels[], unsigned nchannels);
      ~FFT();
    public:
      void save(char*& p) const;
      void load(const char*& p);
      void snapshot(const QString&) const;
      void save_plots(const QString&) const;
    public:
      void configure(char*& p, unsigned input, unsigned& output,
		     ChannelDefinition* ch[], int* signatures, unsigned nchannels,
		     ConfigureRequest::Source);
      void setup_payload(Cds&);
      void update();
      void initialize(const Ami::DescEntry&);
    public slots:
      void set_channel(int); // set the source
      void plot       ();   // configure the plot
      void remove_plot(QObject*);
    signals:
      void changed();
    private:
      ChannelDefinition** _channels;
      unsigned _nchannels;
      unsigned _channel;
      QComboBox* _parameter;
      Semaphore                  _list_sem;
      std::list<ProjectionPlot*> _plots;
    };
  };
};

#endif
