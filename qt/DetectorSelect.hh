#ifndef AmiQt_DetectorSelect_hh
#define AmiQt_DetectorSelect_hh

#include "ami/qt/QtPWidget.hh"
#include "ami/client/AbsClient.hh"
#include "ami/service/DumpCollector.hh"
#include "ami/service/Pool.hh"

#include "pdsdata/xtc/DetInfo.hh"

#include <QtCore/QString>
#include <list>

class QPrinter;
class QLabel;
class QListWidget;
class QListWidgetItem;
class QTimer;
class QGroupBox;
class QFileDialog;

namespace Ami {
  class ClientManager;
  class ConnectionManager;
  namespace Qt {
    class AbsClient;
    class QtTopWidget;
    class Filter;
    class FilterSetup;
    class RateDisplay;
    class DetectorSelect : public QtPWidget,
			   public Ami::AbsClient {
      Q_OBJECT
    public:
      DetectorSelect(const QString&,
                     unsigned ppinterface,
                     unsigned interface,
                     unsigned serverGroup,
                     QWidget* guestBox = NULL,
                     bool quiet = false);
      ~DetectorSelect();
    public:
      void connected       () ;
      int  configure       (iovec*) ;
      int  configured      () ;
      void discovered      (const DiscoveryRx&) ;
      void beginRun        (unsigned);
      void endRun          (unsigned);
      int  read_description(Socket&,int) ;
      int  read_payload    (Socket&,int) ;
      bool svc             () const;
      void process         () ;
      void disconnected    ();
    public:
      int                 get_setup(char*) const;
      void                set_setup(const char*,int);
    public slots:
      void save_setup();
      void load_setup();
      void load_setup(const char*);
      void print_setup();
      void default_setup();

      void reset_plots();
      void save_plots();
      void queue_autosave();
      void autosave();
      void autoload();
      void run_snapshot();

      void set_filters();
      void l3t_export();

      void show_detector(QListWidgetItem*);
      void change_detectors (const char*);
    signals:
      void detectors_discovered (const char*);
      void _reset_plots();
    private:
      Ami::Qt::AbsClient* _create_client (const Pds::Src&, unsigned, const QString&, const char*&);
      void                _connect_client(Ami::Qt::AbsClient* client);
//       void                _update_groups();

      void setDiscovered(bool isDiscovered);
      void waitForDiscovered();
    private:
      bool           _quiet;
      unsigned       _interface;
      unsigned       _serverGroup;
      unsigned       _connect_id;
      ConnectionManager* _connect_mgr;
      ClientManager* _manager;
      FilterSetup*   _filters;
      Pool           _request;
      std::list<QtTopWidget*> _client;
      QListWidget*   _detList;
      QPrinter*      _printer;
      QTimer*        _autosave_timer;
      QTimer*        _snapshot_timer;
      RateDisplay*   _rate_display;
      pthread_mutex_t _mutex;
      pthread_cond_t _condition;
      bool           _discovered;
      Filter*        _filter_export;
      bool           _l3t_export;
      QFileDialog*   _l3t_export_file;
      DumpCollector  _dump;
    };
  };
};

#endif

