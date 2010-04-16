#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/client/AbsClient.hh"
#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <list>

class QPrinter;
class QListWidget;
class QListWidgetItem;
class QTimer;

namespace Ami {
  class ClientManager;
  namespace Qt {
    class AbsClient;
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
    public:
      int                 get_setup(char*) const;
      void                set_setup(const char*,int);
    public slots:
      void save_setup();
      void load_setup();
      void print_setup();

      void reset_plots();
      void save_plots();
      void queue_autosave();
      void autosave();
      void autoload();

      void show_detector(QListWidgetItem*);
      void change_detectors (const char*);
    signals:
      void detectors_discovered (const char*);

    private:
      void                _load_setup_from_file(const char*);
      Ami::Qt::AbsClient* _create_client (const Pds::DetInfo&, unsigned);
      void                _connect_client(Ami::Qt::AbsClient* client);
      void                _update_groups();

    private:
      unsigned       _interface;
      unsigned       _serverGroup;
      unsigned short _clientPort;
      ClientManager* _manager;
      std::list<QtTopWidget*> _client;
      QListWidget*   _detList;
      QPrinter*      _printer;
      DetectorReset* _reset_box;
      DetectorSave*  _save_box;
      QTimer*        _autosave_timer;
    };
  };
};

#endif
