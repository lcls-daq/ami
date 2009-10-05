#ifndef AmiQt_EnvClient_hh
#define AmiQt_EnvClient_hh

#include "ami/qt/Requestor.hh"
#include "ami/qt/QtPWidget.hh"
#include "ami/client/AbsClient.hh"

#include "ami/data/Cds.hh"

#include <list>

class QButtonGroup;
class QComboBox;

namespace Ami {
  class VClientManager;
  class DescEntry;
  class Semaphore;

  namespace Qt {
    class Control;
    class Status;
    class EnvPlot;
    class DescTH1F;
    class DescChart;
    class DescProf;
    class EnvClient : public QtPWidget,
		      public Ami::AbsClient,
		      public Requestor {
      Q_OBJECT
    public:
      EnvClient(QWidget*);
      ~EnvClient();
    public:
      void managed         (VClientManager&);
      void request_payload ();
      void one_shot        (bool) {}
    public: // AbsClient interface
      void connected       ();
      int  configure       (iovec*);
      int  configured      ();
      void discovered      (const DiscoveryRx&);
      void read_description(Ami::Socket&);
      void read_payload    (Ami::Socket&);
      void process         ();
    public slots:
      void update_configuration();
      void _read_description   (int);
      void plot                ();
      void remove_plot         (QObject*);
      void change_features ();
    signals:
      void changed();
      void description_changed(int);
    private:
      void _configure(char*& p, 
		      unsigned input, 
		      unsigned& output);
      void _setup_payload(Cds&);
      void _update();
    private:
      unsigned    _input;
      unsigned    _output_signature;
      char*       _request;
      char*       _description;

      Control*    _control;
      Status*     _status;

      Cds             _cds;
      VClientManager* _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      Semaphore*  _sem;

      QButtonGroup* _plot_grp;
      DescTH1F*  _hist;
      DescChart* _vTime;
      DescProf*  _vBld;
      DescProf*  _vFeature;
      QComboBox* _source;
      QComboBox* _features;

      std::list<EnvPlot*> _plots;
    };
  };
};

#endif
