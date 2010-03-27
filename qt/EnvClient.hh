#ifndef AmiQt_EnvClient_hh
#define AmiQt_EnvClient_hh

#include "ami/qt/AbsClient.hh"

#include "ami/data/Cds.hh"

#include <list>

class QButtonGroup;
class QComboBox;

namespace Ami {
  class ClientManager;
  class DescEntry;
  class Semaphore;

  namespace Qt {
    class Control;
    class Status;
    class EnvPlot;
    class ScalarPlotDesc;
    class FeatureBox;
    class EnvClient : public Ami::Qt::AbsClient {
      Q_OBJECT
    public:
      EnvClient(QWidget*, const Pds::DetInfo&, unsigned);
      virtual ~EnvClient();
    public:
      const QString& title() const;
      virtual void save(char*&) const;
      virtual void load(const char*&);
      void save_plots(const QString&) const;
      void reset_plots();
    public:
      void managed         (ClientManager&);
      void request_payload ();
      void one_shot        (bool) {}
    public: // AbsClient interface
      void connected       ();
      int  configure       (iovec*);
      int  configured      ();
      void discovered      (const DiscoveryRx&);
      void read_description(Ami::Socket&,int);
      void read_payload    (Ami::Socket&,int);
      void process         ();
    public slots:
      void update_configuration();
      void _read_description   (int);
      void plot                ();
      void remove_plot         (QObject*);
    signals:
      void description_changed(int);
    private:
      void _configure(char*& p, 
		      unsigned input, 
		      unsigned& output);
      void _setup_payload(Cds&);
      void _update();
    private:
      QString     _title;
      unsigned    _input;
      unsigned    _output_signature;
      char*       _request;
      char*       _description;

      Control*    _control;
      Status*     _status;

      Cds             _cds;
      ClientManager*  _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      Semaphore*  _sem;

      ScalarPlotDesc* _scalar_plot;

      FeatureBox* _source;

      std::list<EnvPlot*> _plots;
    };
  };
};

#endif
