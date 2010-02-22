#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/client/AbsClient.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <list>

class QPrinter;
class QVBoxLayout;

namespace Ami {
  class ClientManager;
  namespace Qt {
    class Client;
    class QtTopWidget;
    class DetectorReset;
    class DetectorSave;
    class DetectorSelect : public QtPWidget,
			   public Ami::AbsClient {
      Q_OBJECT
    public:
      DetectorSelect(const QString&,
		     unsigned interface,
		     unsigned serverGroup);
      ~DetectorSelect();
    public:
      void connected       () ;
      int  configure       (iovec*) ;
      int  configured      () ;
      void discovered      (const DiscoveryRx&) ;
      void read_description(Socket&,int) ;
      void read_payload    (Socket&,int) ;
      void process         () ;
    public slots:
      void save_setup();
      void load_setup();
      void print_setup();

      void reset_plots();
      void save_plots();

      void start_detector(const Pds::DetInfo&, unsigned);
      void start_env  ();

    private:
      Ami::Qt::Client* _create_client(const Pds::DetInfo&, unsigned);

    private:
      unsigned       _interface;
      unsigned       _serverGroup;
      unsigned short _clientPort;
      ClientManager* _manager;
      std::list<QtTopWidget*> _client;
      QVBoxLayout*   _client_layout;
      char*          _restore;
      QPrinter*      _printer;
      DetectorReset* _reset_box;
      DetectorSave*  _save_box;
    };
  };
};

#endif
