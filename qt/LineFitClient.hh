#ifndef AmiQt_LineFitClient_hh
#define AmiQt_LineFitClient_hh

#include "ami/qt/AbsClient.hh"
#include "ami/qt/PostAnalysis.hh"

#include "ami/data/Cds.hh"
#include "ami/data/FeatureCache.hh"
#include "ami/service/Semaphore.hh"
#include "ami/service/Pool.hh"

#include <list>

class QButtonGroup;
class QComboBox;
class QPushButton;
class QLineEdit;

namespace Ami {
  class ClientManager;
  class DescEntry;

  namespace Qt {
    class Control;
    class Status;
    class EnvPlot;
    class EnvOverlay;
    class EnvTable;
    class LineFitPlotDesc;
    class Filter;
    class SharedData;
    class LineFitClient : public Ami::Qt::AbsClient,
			  public PostAnalysis {
      Q_OBJECT
    public:
      LineFitClient(QWidget*, const Pds::DetInfo&, unsigned, const QString&);
      virtual ~LineFitClient();
    public:
      const QString& title() const;
      virtual void save(char*&) const;
      virtual void load(const char*&);
      void save_plots(const QString&) const;
      void reset_plots();
    public:
      void managed         (ClientManager&);
      void request_payload ();
      void one_shot        (bool);
    public: // AbsClient interface
      void connected       ();
      int  configure       (iovec*);
      int  configured      ();
      void discovered      (const DiscoveryRx&);
      int  read_description(Ami::Socket&,int);
      int  read_payload    (Ami::Socket&,int);
      bool svc             () const;
      void process         ();
    public slots:
      void update_configuration();
      void _read_description   (int);
      void plot                ();
      void overlay             ();
      void table               ();
      void remove_plot         (QObject*);
      void remove_table        (QObject*);
      void select_xsource      ();
      void select_ysource      ();
    signals:
      void description_changed(int);
    private:
      void _select_source(QLineEdit*);
      void _configure(char*& p, 
		      unsigned input, 
		      unsigned& output);
      void _setup_payload(Cds&);
      void _update();
    private:
      QString     _title;
      unsigned    _input;
      Ami::ScalarSet _set;

      unsigned    _output_signature;
      Pool        _request;
      Pool        _description;

      Control*    _control;
      Status*     _status;

      Cds             _cds;
      ClientManager*  _manager;
      unsigned        _niovload;
      iovec*          _iovload;

      Semaphore*  _sem;

      bool _throttled;

      QLineEdit*   _xsource_edit;
      QPushButton* _xsource_compose;
      QLineEdit*   _ysource_edit;
      QPushButton* _ysource_compose;

      Filter*      _filter;

      LineFitPlotDesc* _scalar_plot;

      Semaphore            _list_sem;
      std::list<EnvPlot*>  _plots;
      std::list<EnvTable*> _tabls;

    public:
      void plot(const QString&, DescEntry*, SharedData*);

    public:
      void add_overlay   (DescEntry*, QtPlot*, SharedData*);
      void remove_overlay(QtOverlay*);
    private:
      std::list<EnvOverlay*> _ovls;

      bool _reset;
    };
  };
};

#endif
